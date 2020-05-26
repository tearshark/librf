//依赖 https://github.com/tearshark/modern_cb 项目
//依赖 https://github.com/tearshark/librf 项目
//依赖 https://github.com/qicosmos/cinatra 项目

#include <iostream>
#include "../../cinatra/include/cinatra.hpp"
#include "librf.h"
#include "use_librf.h"

using namespace resumef;
using namespace cinatra;

void test_async_cinatra_client()
{
	std::string uri = "http://www.baidu.com";
	std::string uri1 = "http://cn.bing.com";
	std::string uri2 = "https://www.baidu.com";
	std::string uri3 = "https://cn.bing.com";

	GO
	{
		auto client = cinatra::client_factory::instance().new_client();
		response_data data = co_await client->async_get(uri, use_librf);
		print(data.resp_body);
	};

	GO
	{
		auto client = cinatra::client_factory::instance().new_client();
		response_data data = co_await client->async_get(uri1, use_librf);
		print(data.resp_body);
	};

	GO
	{
		auto client = cinatra::client_factory::instance().new_client();
		response_data data = co_await client->async_post(uri, "hello", use_librf);
		print(data.resp_body);
	};

#ifdef CINATRA_ENABLE_SSL
	GO
	{
		auto client = cinatra::client_factory::instance().new_client();
		response_data data = co_await client->async_get(uri2, use_librf);
		print(data.resp_body);
	};

	GO
	{
		auto client = cinatra::client_factory::instance().new_client();
		response_data data = co_await client->async_get(uri3, use_librf);
		print(data.resp_body);
	};
#endif

	resumef::this_scheduler()->run_until_notask();
}
