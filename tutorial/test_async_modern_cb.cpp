
#include "librf.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <future>

template<typename _Input_t, typename _Callable_t>
__declspec(noinline)
void tostring_async_originalism(_Input_t&& value, _Callable_t&& callback)
{
	std::thread([callback = std::move(callback), value = std::forward<_Input_t>(value)]
		{
			callback(std::to_string(value));
		}).detach();
}

//回调适配器的模板类
//这个默认类以_Callable_t作为真正的回调
//返回无意义的int，以便于编译通过
template<typename _Callable_t, typename _Result_t>
struct modern_callback_adapter_t
{
	using return_type = int;
	using callback_type = _Callable_t;

	static std::tuple<callback_type, return_type> traits(_Callable_t&& callback)
	{
		return { std::forward<_Callable_t>(callback), 0 };
	}
};

#define MODERN_CALLBACK_TRAITS(type) \
	using _Result_t = type; \
	using _Adapter_t = modern_callback_adapter_t<std::decay_t<_Callable_t>, _Result_t>; \
	auto adapter = typename _Adapter_t::traits(std::forward<_Callable_t>(callback))
#define MODERN_CALLBACK_CALL() callback = std::move(std::get<0>(adapter))
#define MODERN_CALLBACK_RETURN() return std::move(std::get<1>(adapter)) 

template<typename _Input_t, typename _Callable_t>
auto tostring_async_macro(_Input_t&& value, _Callable_t&& callback)
{
	MODERN_CALLBACK_TRAITS(std::string);

	std::thread([MODERN_CALLBACK_CALL(), value = std::forward<_Input_t>(value)]
		{
			callback(std::to_string(value));
		}).detach();

	MODERN_CALLBACK_RETURN();
}

//演示如何通过回调适配器模型，将异步回调，扩展到支持future模式，调用链模式，以及协程。
//
//一个使用回调处理结果的异步函数，会涉及五个概念：
//_Input_t：异步函数的输入参数；
//_Callable_t：回调函数，这个回调，必须调用一次，且只能调用一次；
//_Return_t：异步函数的返回值；
//_Result_t：异步函数完成后的结果值，作为回调函数的入参部分；这个参数可以有零至多个；
//_Exception_t：回调函数的异常， 如果不喜欢异常的则忽略这个部分，但就得异步代码将异常处置妥当；
//
//在回调适配器模型里，_Input_t/_Result_t/_Exception_t(可选)是异步函数提供的功能所固有的部分；_Callable_t/_Return_t
//部分并不直接使用，而是通过适配器去另外处理。这样给予适配器一次扩展到future模式，调用链模式的机会，以及支持协程的机会。
//
//tostring_async 演示了在其他线程里，将_Input_t的输入值，转化为std::string类型的_Result_t。
//然后调用void(std::string &&)类型的_Callable_t。忽视异常处理。
//
template<typename _Input_t, typename _Callable_t>
__declspec(noinline)
auto tostring_async(_Input_t&& value, _Callable_t&& callback)
//-> typename modern_callback_adapter_t<std::decay_t<_Callable_t>, std::string>::return_type
{
	using _Result_t = std::string;
	//适配器类型
	using _Adapter_t = modern_callback_adapter_t<std::decay_t<_Callable_t>, _Result_t>;
	//通过适配器获得兼容_Callable_t类型的真正的回调，以及返回值_Return_t
	auto adapter = typename _Adapter_t::traits(std::forward<_Callable_t>(callback));

	//real_callback与callback未必是同一个变量，甚至未必是同一个类型
	std::thread([real_callback = std::move(std::get<0>(adapter)), value = std::forward<_Input_t>(value)]
		{
			real_callback(std::to_string(value));
		}).detach();

	//返回适配器的return_type变量
	return std::move(std::get<1>(adapter));
}

template<typename _Input_t>
auto tostring_async_future(_Input_t&& value)
{
	std::promise<std::string> _promise;
	std::future<std::string> _future = _promise.get_future();

	std::thread([_promise = std::move(_promise), value = std::forward<_Input_t>(value)]() mutable
		{
			_promise.set_value(std::to_string(value));
		}).detach();

	return std::move(_future);
}

//-------------------------------------------------------------------------------------------------
//下面演示如何扩展tostring_async函数，以支持future模式

//一、做一个辅助类
struct use_future_t {};
//二、申明这个辅助类的全局变量。不什么这个变量也行，就是每次要写use_future_t{}，麻烦些
//以后就使用use_future，替代tostring_async的callback参数了。
//这个参数其实不需要实质传参，最后会被编译器优化没了。
//仅仅是要指定_Callable_t的类型为use_future_t，
//从而在tostring_async函数内，使用偏特化的modern_callback_adapter_t<use_future_t, ...>而已。
inline constexpr use_future_t use_future{};

//将替换use_future_t的，真正的回调类。
//此回调类，符合tostring_async的_Callable_t函数签名。生成此类的实例作为real_callback交给tostring_async作为异步回调。
//
//future模式下，此类持有一个std::promise<_Result_t>，便于设置值和异常
//而将与promise关联的future作为返回值_Return_t，让tostring_async返回。
template<typename _Result_t>
struct use_future_callback_t
{
	using promise_type = std::promise<_Result_t>;

	mutable promise_type _promise;

	void operator()(_Result_t&& value) const
	{
		_promise.set_value(value);
	}

	void operator()(_Result_t&& value, std::exception_ptr&& eptr) const
	{
		if (eptr != nullptr)
			_promise.set_exception(std::forward<std::exception_ptr>(eptr));
		else
			_promise.set_value(std::forward<_Result_t>(value));
	}
};

//偏特化_Callable_t为use_future_t类型的modern_callback_adapter_t
//真正的回调类型是use_future_callback_t，返回类型_Return_t是std::future<_Result_t>。
//配合use_future_callback_t的std::promise<_Result_t>，正好组成一对promise/future对。
//promise在真正的回调里设置结果值；
//future返回给调用者获取结果值。
template<typename _Result_t>
struct modern_callback_adapter_t<use_future_t, _Result_t>
{
	using return_type = std::future<_Result_t>;
	using callback_type = use_future_callback_t<_Result_t>;

	static std::tuple<callback_type, return_type> traits(const use_future_t&/*没人关心这个变量*/)
	{
		callback_type real_callback{};
		return_type future = real_callback._promise.get_future();

		return { std::move(real_callback), std::move(future) };
	}
};

//-------------------------------------------------------------------------------------------------

//同理，可以制作支持C++20的协程的下列一系列类（其实，这才是我的最终目的）
struct use_awaitable_t {};
inline constexpr use_awaitable_t use_awaitable{};

template<typename _Result_t>
struct use_awaitable_callback_t
{
	using promise_type = resumef::promise_t<_Result_t>;
	using state_type = typename promise_type::state_type;

	resumef::counted_ptr<state_type> _state;

	void operator()(_Result_t&& value) const
	{
		_state->set_value(std::forward<_Result_t>(value));
	}
	void operator()(_Result_t&& value, std::exception_ptr&& eptr) const
	{
		if (eptr != nullptr)
			_state->set_exception(std::forward<std::exception_ptr>(eptr));
		else
			_state->set_value(std::forward<_Result_t>(value));
	}
};

template<typename _Result_t>
struct modern_callback_adapter_t<use_awaitable_t, _Result_t>
{
	using promise_type = resumef::promise_t<_Result_t>;
	using return_type = resumef::future_t<_Result_t>;
	using callback_type = use_awaitable_callback_t<_Result_t>;

	static std::tuple<callback_type, return_type> traits(const use_awaitable_t&)
	{
		promise_type promise;

		return { callback_type{ promise._state }, promise.get_future() };
	}
};

//所以，我现在的看法是，支持异步操作的库，尽可能如此设计回调。这样便于支持C++20的协程。以及future::then这样的任务链。
//这才是“摩登C++”！


//使用范例
__declspec(noinline)
void resumable_main_modern_cb()
{
	using namespace std::literals;

	//使用lambda作为异步回调函数，传统用法
	//tostring_async_originalism(-1.0, [](std::string && value)
	//	{
	//		std::cout << value << std::endl;
	//	});

	tostring_async(1.0, [](std::string && value)
		{
			std::cout << value << std::endl;
		});
/*
	std::cout << nocare << std::endl;
	std::this_thread::sleep_for(1s);
	std::cout << "......" << std::endl;

	//支持future的用法
	std::future<std::string> f1 = tostring_async_future(5);
	std::cout << f1.get() << std::endl;

	std::future<std::string> f2 = tostring_async(6.0f, use_future);
	std::cout << f2.get() << std::endl;


	//支持librf的用法
	GO
	{
		std::string result = co_await tostring_async(10.0, use_awaitable);
		std::cout << result << std::endl;
	};
	resumef::this_scheduler()->run_until_notask();
*/
}
