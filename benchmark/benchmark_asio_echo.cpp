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

intptr_t g_echo_count = 0;

future_vt RunEchoSession(tcp::socket socket)
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

				++g_echo_count;
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
			go[&, idx]() -> future_vt
			{
				for (;;)
				{
					try
					{
						co_await acceptor.async_accept(socketes.c[idx], rf_task);
						go RunEchoSession(std::move(socketes.c[idx]));
					}
					catch (std::exception e)
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

void resumable_main_benchmark_asio_server()
{
	using namespace std::literals;

	asio::io_service io_service;
	tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 3456));
	uarray<tcp::socket, 128> socketes(acceptor.get_io_service());

	AcceptConnections(acceptor, socketes);

	GO
	{
		for (;;)
		{
			g_echo_count = 0;
			co_await 1s;
			std::cout << g_echo_count << std::endl;
		}		
	};

	for (;;)
	{
		io_service.poll();
		this_scheduler()->run_one_batch();
	}
}

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
			[this, self](std::error_code ec, tcp::resolver::iterator iter)
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
			[this, self](const asio::error_code& ec, std::size_t size)
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

void resumable_main_benchmark_asio_client(intptr_t nNum)
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