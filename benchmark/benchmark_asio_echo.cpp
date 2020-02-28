#include <SDKDDKVer.h>

/*
#include <chrono>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
*/

#include <asio.hpp>
#include "librf.h"
#include "src/asio_task.h"

#pragma warning(disable : 4834)

using namespace asio;
using namespace asio::ip;

using namespace resumef;

template<class _Ty, size_t _Size>
union uarray
{
	std::array<_Ty, _Size>		c;

	template<class... Args>
	uarray(Args&... args)
	{
		for (auto & v : c)
			new(&v) _Ty(args...);
	}
	~uarray()
	{
		for (auto & v : c)
			v.~_Ty();
	}
};

#define BUF_SIZE 1024

std::atomic<intptr_t> g_echo_count = 0;

future_t<> RunEchoSession(tcp::socket socket)
{
	std::size_t bytes_transferred = 0;
	std::array<char, BUF_SIZE> buffer;
	for(;;)
	{
		try
		{
			bytes_transferred += co_await socket.async_read_some(asio::buffer(buffer.data() + bytes_transferred, buffer.size() - bytes_transferred), rf_task);
			if (bytes_transferred >= buffer.size())
			{
				co_await asio::async_write(socket, asio::buffer(buffer, buffer.size()), rf_task);
				bytes_transferred = 0;

				g_echo_count.fetch_add(1, std::memory_order_release);
			}
		}
		catch (std::exception & e)
		{
			std::cerr << e.what() << std::endl;
			break;
		}
	}
}

template<size_t _N>
void AcceptConnections(tcp::acceptor & acceptor, uarray<tcp::socket, _N> & socketes)
{
	try
	{
		for (size_t idx = 0; idx < socketes.c.size(); ++idx)
		{
			go[&, idx]() -> future_t<>
			{
				for (;;)
				{
					try
					{
						co_await acceptor.async_accept(socketes.c[idx], rf_task);
						go RunEchoSession(std::move(socketes.c[idx]));
					}
					catch (std::exception & e)
					{
						std::cerr << e.what() << std::endl;
					}
				}
			};
		}
	}
	catch (std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void StartPrintEchoCount()
{
	using namespace std::literals;

	GO
	{
		for (;;)
		{
			g_echo_count.exchange(0, std::memory_order_release);
			co_await 1s;
			std::cout << g_echo_count.load(std::memory_order_acquire) << std::endl;
		}
	};
}

void RunOneBenchmark(bool bMain)
{
	resumef::local_scheduler ls;

	asio::io_service io_service;
	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 3456));
	uarray<tcp::socket, 16> socketes(io_service);

	AcceptConnections(acceptor, socketes);
	if (bMain) StartPrintEchoCount();

	for (;;)
	{
		io_service.poll();
		this_scheduler()->run_one_batch();
	}
}

void resumable_main_benchmark_asio_server()
{
#if RESUMEF_ENABLE_MULT_SCHEDULER
	std::array<std::thread, 2> thds;
	for (size_t i = 0; i < thds.size(); ++i)
	{
		thds[i] = std::thread(&RunOneBenchmark, i == 0);
	}

	for (auto & t : thds)
		t.join();
#else
	RunOneBenchmark(true);
#endif
}

//----------------------------------------------------------------------------------------------------------------------

future_t<> RunPipelineEchoClient(asio::io_service & ios, tcp::resolver::iterator ep)
{
	std::shared_ptr<tcp::socket> sptr = std::make_shared<tcp::socket>(ios);

	try
	{
		co_await asio::async_connect(*sptr, ep, rf_task);

		GO
		{
			std::array<char, BUF_SIZE> write_buff_;
			for (auto & c : write_buff_)
				c = 'A' + rand() % 52;

			try
			{
				for (;;)
				{
					co_await asio::async_write(*sptr, asio::buffer(write_buff_), rf_task);
				}
			}
			catch (std::exception & e)
			{
				std::cerr << e.what() << std::endl;
			}
		};

		GO
		{
			try
			{
				std::array<char, BUF_SIZE> read_buff_;
				for (;;)
				{
					co_await sptr->async_read_some(asio::buffer(read_buff_), rf_task);
				}
			}
			catch (std::exception & e)
			{
				std::cerr << e.what() << std::endl;
			}
		};
	}
	catch (std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}
}

#if _HAS_CXX17

future_t<> RunPingPongEchoClient(asio::io_service & ios, tcp::resolver::iterator ep)
{
	tcp::socket socket_{ ios };

	std::array<char, BUF_SIZE> read_buff_;
	std::array<char, BUF_SIZE> write_buff_;

	try
	{
		co_await asio::async_connect(socket_, ep, rf_task);

		for (auto & c : write_buff_)
			c = 'A' + rand() % 52;

		for (;;)
		{
			co_await when_all(
				asio::async_write(socket_, asio::buffer(write_buff_), rf_task),
				socket_.async_read_some(asio::buffer(read_buff_), rf_task)
			);
		}
	}
	catch (std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void resumable_main_benchmark_asio_client_with_rf(intptr_t nNum)
{
	nNum = std::max((intptr_t)1, nNum);

	try
	{
		asio::io_service ios;

		asio::ip::tcp::resolver resolver_(ios);
		asio::ip::tcp::resolver::query query_("localhost", "3456");
		tcp::resolver::iterator iter = resolver_.resolve(query_);

		for (intptr_t i = 0; i < nNum; ++i)
		{
			go RunPingPongEchoClient(ios, iter);
		}

		for (;;)
		{
			ios.poll();
			this_scheduler()->run_one_batch();
		}
	}
	catch (std::exception & e)
	{
		std::cout << e.what() << std::endl;
	}
}
#endif

class chat_session : public std::enable_shared_from_this<chat_session>
{
public:
	chat_session(asio::io_service & ios, tcp::resolver::iterator ep)
		: socket_(ios)
		, endpoint_(ep)
	{
	}

	void start()
	{
		do_connect();
	}

private:
	void do_connect()
	{
		auto self = this->shared_from_this();
		asio::async_connect(socket_, endpoint_,
			[this, self](std::error_code ec, tcp::resolver::iterator )
			{
				if (!ec)
				{
					for (auto & c : write_buff_)
						c = 'A' + rand() % 52;

					do_write();
				}
				else
				{
					std::cerr << ec.message() << std::endl;
				}
			});
	}

	void do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(asio::buffer(read_buff_),
			[this, self](const asio::error_code& ec, std::size_t )
			{
				if (!ec)
				{
					do_write();
				}
				else
				{
					std::cerr << ec.message() << std::endl;
				}
		});
	}

	void do_write()
	{
		auto self(shared_from_this());
		asio::async_write(socket_,
			asio::buffer(write_buff_),
			[this, self](std::error_code ec, std::size_t)
			{
				if (!ec)
				{
					do_read();
				}
				else
				{
					std::cerr << ec.message() << std::endl;
				}
		});
	}

	tcp::socket socket_;
	tcp::resolver::iterator endpoint_;

	std::array<char, BUF_SIZE> read_buff_;
	std::array<char, BUF_SIZE> write_buff_;
};

void resumable_main_benchmark_asio_client_with_callback(intptr_t nNum)
{
	nNum = std::max((intptr_t)1, nNum);

	try
	{
		asio::io_service ios;

		asio::ip::tcp::resolver resolver_(ios);
		asio::ip::tcp::resolver::query query_("127.0.0.1", "3456");
		tcp::resolver::iterator iter = resolver_.resolve(query_);

		for (intptr_t i = 0; i < nNum; ++i)
		{
			auto chat = std::make_shared<chat_session>(ios, iter);
			chat->start();
		}

		ios.run();
	}
	catch (std::exception & e)
	{
		std::cout << "Exception: " << e.what() << "\n";
	}
}

void resumable_main_benchmark_asio_client(intptr_t nNum)
{
	resumable_main_benchmark_asio_client_with_callback(nNum);
}