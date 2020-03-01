
#include "librf.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <future>

//原旨主义的异步函数，其回调写法大致如下
template<typename _Input_t, typename _Callable_t>
__declspec(noinline)
void tostring_async_originalism(_Input_t&& value, _Callable_t&& token)
{
	std::thread([callback = std::move(token), value = std::forward<_Input_t>(value)]
		{
			callback(std::to_string(value));
		}).detach();
}

//使用原旨主义的方式扩展异步方法来支持future
template<typename _Input_t>
auto tostring_async_originalism_future(_Input_t&& value)
{
	std::promise<std::string> _promise;
	std::future<std::string> _future = _promise.get_future();

	std::thread([_promise = std::move(_promise), value = std::forward<_Input_t>(value)]() mutable
	{
		_promise.set_value(std::to_string(value));
	}).detach();

	return std::move(_future);
}

//----------------------------------------------------------------------------------------------------------------------
//以下演示如何通过现代回调(Modern Callback)， 使用回调适配器模型，
//将异步回调函数扩展到支持future模式，调用链模式，以及协程。

//首先，准备modern_call_return_void_t和modern_callback_adapter_t给异步函数使用

//通过一个间接的类来解决返回void的语法问题，以便于优化返回值
struct modern_call_return_void_t
{
	void get(){}
};

//回调适配器的模板类
//_Callable_t 要符合 _Signature_t 签名
//这个类除了转移token外，不做任何有效的工作
//有效工作等待特列化的类去做
template<typename _Callable_t, typename _Signature_t>
struct modern_callback_adapter_t
{
	using callback_type = _Callable_t;
	using return_type = modern_call_return_void_t;

	static std::tuple<callback_type, return_type> traits(_Callable_t&& token)
	{
		return { std::forward<_Callable_t>(token), {} };
	}
};


//一个使用回调处理结果的异步函数，会涉及以下概念：
//_Input_t：异步函数的输入参数；
//_Signature_t: 此异步回调的函数签名；应当满足‘void(_Exception_t, _Result_t...)’或者‘void(_Result_t...)’类型；
//_Callable_t：回调函数或标记，如果是回调函数，则需要符合_Signature_t的签名类型。这个回调，必须调用一次，且只能调用一次；
//_Return_t：异步函数的返回值；
//_Result_t...：异步函数完成后的结果值，作为回调函数的入参部分；这个参数可以有零至多个；
//_Exception_t：回调函数的异常， 如果不喜欢异常的则忽略这个部分，但就得异步代码将异常处置妥当；
//
//在回调适配器模型里，_Input_t/_Result_t/_Exception_t(可选)是异步函数提供的功能所固有的部分；_Callable_t/_Return_t
//部分并不直接使用，而是通过适配器去另外处理。这样给予适配器一次扩展到future模式，调用链模式的机会，以及支持协程的机会。
//
//tostring_async 演示了在其他线程里，将_Input_t的输入值，转化为std::string类型的_Result_t。
//然后调用_Signature_t为 ‘void(std::string &&)’ 类型的 _Callable_t。
//忽视异常处理，故没有_Exception_t。
//
template<typename _Input_t, typename _Callable_t>
__declspec(noinline)
auto tostring_async(_Input_t&& value, _Callable_t&& token)
{
	//适配器类型
	using _Adapter_t = modern_callback_adapter_t<typename resumef::remove_cvref_t<_Callable_t>, void(std::string)>;
	//通过适配器获得兼容_Signature_t类型的真正的回调，以及返回值_Return_t
	auto adapter = _Adapter_t::traits(std::forward<_Callable_t>(token));

	//callback与token未必是同一个变量，甚至未必是同一个类型
	std::thread([callback = std::move(std::get<0>(adapter)), value = std::forward<_Input_t>(value)]
		{
			using namespace std::literals;
			std::this_thread::sleep_for(0.1s);
			callback(std::to_string(value));
		}).detach();

	//返回适配器的_Return_t变量
	return std::move(std::get<1>(adapter)).get();
}

//或者宏版本写法
#define MODERN_CALLBACK_TRAITS(_Token_value, _Signature_t) \
	using _Adapter_t = modern_callback_adapter_t<typename resumef::remove_cvref_t<_Callable_t>, _Signature_t>; \
	auto _Adapter_value = _Adapter_t::traits(std::forward<_Callable_t>(_Token_value))
#define MODERN_CALLBACK_CALL() std::move(std::get<0>(_Adapter_value))
#define MODERN_CALLBACK_RETURN() return std::move(std::get<1>(_Adapter_value)).get()

template<typename _Input_t, typename _Callable_t>
auto tostring_async_macro(_Input_t&& value, _Callable_t&& token)
{
	MODERN_CALLBACK_TRAITS(token, void(std::string));

	std::thread([callback = MODERN_CALLBACK_CALL(), value = std::forward<_Input_t>(value)]
		{
			callback(std::to_string(value));
		}).detach();

	MODERN_CALLBACK_RETURN();
}

//----------------------------------------------------------------------------------------------------------------------
//下面演示如何扩展tostring_async函数，以支持future模式
//future库有多种，但应当都提供遵循promise/future对，兼容std::promise/std::future用法
//这样的话，可以做一个更加通用的支持future的callback类

//实现use_future_callback_t的基类，避免写一些重复代码
template<typename _Promise_traits, typename _Result_t>
struct use_future_callback_base_t
{
	//回调函数的结果类型，已经排除掉了异常参数
	using result_type = _Result_t;

	//通过_Promise_traits获取真正的promise类型
	using promise_type = typename _Promise_traits::template promise_type<result_type>;

	//此类持有一个std::promise<_Result_t>，便于设置值和异常
	//而将与promise关联的future作为返回值_Return_t，让tostring_async返回。
	mutable promise_type _promise;

	auto get_future() const
	{
		return this->_promise.get_future();
	}
};

//此类的实例作为真正的callback，交给异步回调函数，替换token。
//在实际应用中，需要针对是否有异常参数，结果值为0，1，多个等情况做特殊处理，故还需要通过更多的偏特化版本来支持。
//具体的异常参数，需要根据实际应用去特里化。这里仅演示通过std::exception_ptr作为异常传递的情况。
template<typename...>
struct use_future_callback_t;

//无异常，无结果的callback类型：void()
template<typename _Promise_traits>
struct use_future_callback_t<_Promise_traits> : public use_future_callback_base_t<_Promise_traits, void>
{
	using use_future_callback_base_t<_Promise_traits, void>::use_future_callback_base_t;

	void operator()() const
	{
		this->_promise.set_value();
	}
};

//有异常，无结果的callback类型：void(exception_ptr)
template<typename _Promise_traits>
struct use_future_callback_t<_Promise_traits, std::exception_ptr> : public use_future_callback_base_t<_Promise_traits, void>
{
	using use_future_callback_base_t<_Promise_traits, void>::use_future_callback_base_t;

	void operator()(std::exception_ptr eptr) const
	{
		if (!eptr)
			this->_promise.set_value();
		else
			this->_promise.set_exception(std::move(eptr));
	}
};

//无异常，单结果的callback类型：void(_Result_t)
template<typename _Promise_traits, typename _Result_t>
struct use_future_callback_t<_Promise_traits, _Result_t> : public use_future_callback_base_t<_Promise_traits, _Result_t>
{
	using use_future_callback_base_t<_Promise_traits, _Result_t>::use_future_callback_base_t;

	template<typename Arg>
	void operator()(Arg && arg) const
	{
		this->_promise.set_value(std::forward<Arg>(arg));
	}
};

//有异常，单结果的callback类型：void(std::exception_ptr, _Result_t)
template<typename _Promise_traits, typename _Result_t>
struct use_future_callback_t<_Promise_traits, std::exception_ptr, _Result_t> : public use_future_callback_base_t<_Promise_traits, _Result_t>
{
	using use_future_callback_base_t<_Promise_traits, _Result_t>::use_future_callback_base_t;

	template<typename Arg>
	void operator()(std::exception_ptr eptr, Arg && arg) const
	{
		if (!eptr)
			this->_promise.set_value(std::forward<Arg>(arg));
		else
			this->_promise.set_exception(std::move(eptr));
	}
};

//无异常，多结果的callback类型：void(_Result_t...)
template<typename _Promise_traits, typename... _Result_t>
struct use_future_callback_t<_Promise_traits, _Result_t...> : public use_future_callback_base_t<_Promise_traits, std::tuple<_Result_t...> >
{
	using use_future_callback_base_t<_Promise_traits, std::tuple<_Result_t...> >::use_future_callback_base_t;

	template<typename... Args>
	void operator()(Args&&... args) const
	{
		static_assert(sizeof...(Args) == sizeof...(_Result_t), "");
		this->_promise.set_value(std::make_tuple(std::forward<Args>(args)...));
	}
};

//有异常，多结果的callback类型：void(std::exception_ptr, _Result_t...)
template <typename _Promise_traits, typename... _Result_t>
struct use_future_callback_t<_Promise_traits, std::exception_ptr, _Result_t...> : public use_future_callback_base_t<_Promise_traits, std::tuple<_Result_t...> >
{
	using use_future_callback_base_t<_Promise_traits, std::tuple<_Result_t...> >::use_future_callback_base_t;

	template<typename... Args>
	void operator()(std::exception_ptr eptr, Args&&... args) const
	{
		static_assert(sizeof...(Args) == sizeof...(_Result_t), "");
		if (!eptr)
			this->_promise.set_value(std::make_tuple(std::forward<Args>(args)...));
		else
			this->_promise.set_exception(std::move(eptr));
	}
};



//与use_future_callback_t配套的获得_Return_t的类
template<typename _Future_traits, typename _Result_t>
struct use_future_return_t
{
	using result_type = _Result_t;
	using future_type = typename _Future_traits::template future_type<result_type>;
	future_type _future;

	use_future_return_t(future_type && ft)
		: _future(std::move(ft)) {}

	future_type get()
	{
		return std::move(_future);
	}
};

//利用use_future_callback_t + use_future_return_t 实现的callback适配器
template<typename _Token_as_callable_t, typename... _Result_t>
struct modern_callback_adapter_impl_t
{
	using traits_type = _Token_as_callable_t;
	using callback_type = use_future_callback_t<traits_type, _Result_t...>;
	using result_type = typename callback_type::result_type;
	using return_type = use_future_return_t<traits_type, result_type>;

	static std::tuple<callback_type, return_type> traits(const _Token_as_callable_t& /*没人关心这个变量*/)
	{
		callback_type callback{};
		auto future = callback.get_future();

		return { std::move(callback), std::move(future) };
	}
};

//----------------------------------------------------------------------------------------------------------------------

//一、做一个使用std::promise/std::future的辅助类。
//这个类还负责萃取promise/future对的类型。
struct std_future_t
{
	template<typename _Result_t>
	using promise_type = std::promise<_Result_t>;

	template<typename _Result_t>
	using future_type = std::future<_Result_t>;
};

//二、申明这个辅助类的全局变量。不申明这个变量也行，就是每次要写use_future_t{}，麻烦些。
//以后就使用std_future，替代tostring_async的token参数了。
//这个参数其实不需要实质传参，最后会被编译器优化没了。
//仅仅是要指定_Callable_t的类型为std_future_t，
//从而在tostring_async函数内，使用偏特化的modern_callback_adapter_t<std_future_t, ...>而已。
constexpr std_future_t std_future{};

//三、偏特化_Callable_t为std_future_t类型的modern_callback_adapter_t
//真正的回调类型是use_future_callback_t，返回类型_Return_t是use_future_return_t。
//配合use_future_callback_t的promise<result_type>，和use_future_return_t的future<result_type>，正好组成一对promise/future对。
//promise在真正的回调里设置结果值；
//future返回给调用者获取结果值。
template<typename R, typename... _Result_t>
struct modern_callback_adapter_t<std_future_t, R(_Result_t...)> : public modern_callback_adapter_impl_t<std_future_t, _Result_t...>
{
};

//----------------------------------------------------------------------------------------------------------------------
//同理，可以制作支持C++20的协程的下列一系列类（其实，这才是我的最终目的）
struct use_librf_t
{
	template<typename _Result_t>
	using promise_type = resumef::awaitable_t<_Result_t>;

	template<typename _Result_t>
	using future_type = resumef::future_t<_Result_t>;
};
constexpr use_librf_t use_librf{};

template<typename R, typename... _Result_t>
struct modern_callback_adapter_t<use_librf_t, R(_Result_t...)> : public modern_callback_adapter_impl_t<use_librf_t, _Result_t...>
{
};

//所以，我现在的看法是，支持异步操作的库，尽可能如此设计回调。这样便于支持C++20的协程。以及future::then这样的任务链。
//这才是“摩登C++”！

//----------------------------------------------------------------------------------------------------------------------
//使用范例

//演示异步库有多个异步回调函数，只要按照Modern Callback范式去做回调，就不再需要写额外的代码，就可以适配到future+librf，以及更多的其他库
template<typename _Ty1, typename _Ty2, typename _Callable_t>
auto add_async(_Ty1&& val1, _Ty2&& val2, _Callable_t&& token)
{
	MODERN_CALLBACK_TRAITS(token, void(decltype(val1 + val2)));

	std::thread([=, callback = MODERN_CALLBACK_CALL()]
		{
			using namespace std::literals;
			std::this_thread::sleep_for(0.1s);
			callback(val1 + val2);
		}).detach();

	MODERN_CALLBACK_RETURN();
}

//演示异步库有多个异步回调函数，只要按照Modern Callback范式去做回调，就不再需要写额外的代码，就可以适配到future+librf，以及更多的其他库
template<typename _Ty1, typename _Ty2, typename _Callable_t>
auto muldiv_async(_Ty1&& val1, _Ty2&& val2, _Callable_t&& token)
{
	MODERN_CALLBACK_TRAITS(token, void(std::exception_ptr, decltype(val1 * val2), decltype(val1 / val2)));

	std::thread([=, callback = MODERN_CALLBACK_CALL()]
		{
			using namespace std::literals;
			std::this_thread::sleep_for(0.1s);

			auto v1 = val1 * val2;

			if (val2 == 0)
				callback(std::make_exception_ptr(std::logic_error("divided by zero")), v1, 0);
			else
				callback(nullptr, v1, val1 / val2);
		}).detach();

	MODERN_CALLBACK_RETURN();
}

__declspec(noinline)
void resumable_main_modern_cb()
{
	using namespace std::literals;

	//使用lambda作为异步回调函数，传统用法
	tostring_async_originalism(-1.0, [](std::string && value)
		{
			std::cout << value << std::endl;
		});
	std::this_thread::sleep_for(0.5s);

	tostring_async(1.0, [](std::string && value)
		{
			std::cout << value << std::endl;
		});

	std::this_thread::sleep_for(0.5s);
	std::cout << "......" << std::endl;

	//支持future的用法
	std::future<std::string> f1 = tostring_async_originalism_future(5);
	std::cout << f1.get() << std::endl;

	std::future<std::string> f2 = tostring_async(6.0f, std_future);
	std::cout << f2.get() << std::endl;

	//支持librf的用法
	GO
	{
#ifndef __clang__
		try
#endif
		{
			int val = co_await add_async(1, 2, use_librf);
			std::cout << val << std::endl;

			//muldiv_async函数可能会抛异常，取决于val是否是0
			//异常将会带回到本协程里的代码，所以需要try-catch
			auto ab = co_await muldiv_async(9, val, use_librf);
			//C++17:
			//auto [a, b] = co_await muldiv_async(9, val, use_librf);

			std::string result = co_await tostring_async(std::get<0>(ab) + std::get<1>(ab), use_librf);

			std::cout << result << std::endl;
		}
#ifndef __clang__
		catch (const std::exception& e)
		{
			std::cout << "exception signal : " << e.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "exception signal : who knows?" << std::endl;
		}
#endif
	};

	resumef::this_scheduler()->run_until_notask();
}
