//依赖 https://github.com/tearshark/modern_cb 项目
//依赖 https://github.com/tearshark/librf 项目

#include <future>
#include <string>
#include <iostream>

#include "modern_callback.h"

//原旨主义的异步函数，其回调写法大致如下
template<typename _Input_t, typename _Callable_t>
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
//下面演示如何扩展tostring_async函数，以支持future模式

template<typename _Input_t, typename _Callable_t>
auto tostring_async(_Input_t&& value, _Callable_t&& token)
{
	MODERN_CALLBACK_TRAITS(token, void(std::string));

	std::thread([callback = MODERN_CALLBACK_CALL(), value = std::forward<_Input_t>(value)]
		{
			callback(std::to_string(value));
		}).detach();

	MODERN_CALLBACK_RETURN();
}

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

#include "use_future.h"

static void example_future()
{
	using namespace std::literals;

	//使用lambda作为异步回调函数，传统用法
	tostring_async_originalism(-1.0, [](std::string&& value)
		{
			std::cout << value << std::endl;
		});
	std::this_thread::sleep_for(0.5s);

	tostring_async(1.0, [](std::string&& value)
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
}

#include "librf.h"
#include "use_librf.h"

static void example_librf()
{
	//支持librf的用法
	GO
	{
		try
		{
			int val = co_await add_async(1, 2, use_librf);
			std::cout << val << std::endl;

			//muldiv_async函数可能会抛异常，取决于val是否是0
			//异常将会带回到本协程里的代码，所以需要try-catch
			auto [a, b] = co_await muldiv_async(9, val, use_librf);

			std::string result = co_await tostring_async(a + b, use_librf);

			std::cout << result << std::endl;
		}
		catch (const std::exception & e)
		{
			std::cout << "exception signal : " << e.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "exception signal : who knows?" << std::endl;
		}
	};

	resumef::this_scheduler()->run_until_notask();
}

void resumable_main_modern_cb()
{
	std::cout << __FUNCTION__ << std::endl;
	example_future();
	example_librf();
}