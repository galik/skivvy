#include <iostream>
#include <thread>
#include <string>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <atomic>
#include <future>
#include <chrono>
#include <wordexp.h>

#include <hol/bug.h>
#include <hol/string_utils.h>
#include <hol/small_types.h>
#include <hol/simple_logger.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <boost/asio.hpp>

#define out_con(m) do{std::cout<<m<<'\n';}while(0)

namespace hol {
	using namespace header_only_library::string_utils;
}

using namespace boost;
using namespace header_only_library::small_types::ios;
using namespace header_only_library::small_types::ios::functions;
using namespace header_only_library::small_types::basic;
using namespace header_only_library::small_types::string_containers;
using namespace header_only_library::simple_logger;

//class ssl_iostream
//{
//public:
//	ssl_iostream(): msg_stream(&msg) {}
//	~ssl_iostream() { asio::write(msg_stream, msg); }
//
//	template<typename T> friend
//	ssl_iostream& operator<<(ssl_iostream& io, T const& v)
//	{
//		io.msg_stream << v;
//		io.msg_stream.flush();
//		return io;
//	}
//
//private:
//	boost::asio::streambuf msg;
//	std::ostream msg_stream;
//};

struct malloc_deleter
{
	template <class T>
	void operator()(T* p) { std::free(p); }
};

using cstring_uptr = std::unique_ptr<char, malloc_deleter>;

str wordexp(str var, int flags = 0)
{
	wordexp_t p;
	if(!wordexp(var.c_str(), &p, flags))
	{
		if(p.we_wordc && p.we_wordv[0])
			var = p.we_wordv[0];
		wordfree(&p);
	}
	return var;
}

namespace this_thread {

class out
: public std::ostream
{
	std::mutex mtx;
	std::ostream& os;

public:
	out(std::ostream& os): os(os) {}

	template<typename Type>
	std::ostream& operator<<(const Type& type)
	{
		std::lock_guard<std::mutex> lock(mtx);
		return os << type;
	}
};

}

class SkivvyClient
{
	using clock = std::chrono::system_clock;

	str config_dir;
	str host = "localhost";
	str port = "7334";

	this_thread::out os;

	std::mutex send_mtx;

	struct recv_rv
	{
		boost::system::error_code ec;
		str msg;
		explicit operator bool() const { return !ec; };
	};

	static
	recv_rv recv(asio::ip::tcp::socket& sock)
	{
		recv_rv rv;
		std::array<char, 1024> buf;
		while(auto len = sock.read_some(asio::buffer(buf), rv.ec))
		{
			rv.msg.append(buf.data(), len);
			if(len < buf.size())
				break;
		}
		return rv;
	}

	static
	boost::system::error_code send(asio::ip::tcp::socket& sock, str msg)
	{
		boost::system::error_code ec;
		auto pos = msg.data();
		auto end = pos + msg.size();
		while(!ec && (pos += sock.write_some(asio::buffer(pos, end - pos), ec)) < end) {}
		return ec;
	}

	bool send(const str& cmd, str& res, bool report = true)
	{
		std::lock_guard<std::mutex> lock(send_mtx);

		asio::io_service io_service;
		asio::ip::tcp::socket ss(io_service);
		boost::system::error_code ec;
		asio::ip::tcp::resolver resolver(io_service);
		asio::connect(ss, resolver.resolve({host, port}), ec);

		if(ec)
		{
			if(report)
				os << "error: " << ec.message() << std::endl;
			return false;
		}

		if(!(ec = send(ss, cmd + "\0")))
		{
			if(auto rv = recv(ss))
				res = rv.msg;
			else
				ec = rv.ec;
		}

		return !ec;
	}

	std::atomic<bool> done;
	std::atomic<bool> dusted;
	std::future<void> fut;

public:
	SkivvyClient(const str& config_dir, const str& host = "localhost", const str& port = "7334")
	: config_dir(config_dir), host(host), port(port), os(std::cout)
	, done(false), dusted(false)
	{
	}

	using str_vec_map = std::map<str, str_vec>;

	str_vec parse_status(str&& line)
	{
		str_vec status;
		siss iss(line);
		while(sgl(iss, line))
			status.push_back(std::move(line));
		return status;
	}

//	void poll()
//	{
//		str line;
//		while(!done)
//		{
//			if(send("/get_status", line))
//				for(auto&& l: parse_status(std::move(line)))
//					if(!hol::trim_mute(l).empty())
//						os << l << std::endl;
//			std::this_thread::sleep_for(std::chrono::milliseconds(500));
//		}
//		dusted = true;
//	}

	void run()
	{
		using_history();
		read_history((config_dir + "/.history").c_str());

		str name;
		str chan;

		str shell_prompt = "unknown: ";

		if(send("/botnick", name))
			shell_prompt = name + ": ";

		str line, prev;
		cstring_uptr input;

		input.reset(readline(shell_prompt.c_str()));

		while(line != "/die" && input && hol::trim_mute(line = input.get()) != "exit")
		{
			if(!line.empty())
			{
				if(line != prev)
				{
					add_history(line.c_str());
					write_history((config_dir + "/.history").c_str());
					prev = line;
				}

				str res;
				if(send(line, res) && !hol::trim_mute(res).empty())
					os << res << std::endl;

				if(send("/botnick", name))
					shell_prompt = "<" + name + ">: ";
			}
			input.reset(readline(shell_prompt.c_str()));
		}

		done = true;

		clock::time_point timeout = clock::now() + std::chrono::seconds(10);

		while(!dusted && timeout < clock::now())
			std::this_thread::sleep_for(std::chrono::seconds(1));

		if(!dusted)
			LOG::E << "client timed out waiting for response";

		if(fut.valid())
			fut.get();
	}
};

int main(int argc, char* argv[])
{
	std::cout << "Skivvy Client:" << '\n';

	const str_vec args(argv + 1, argv + argc);

	str port = "7334";
	str host = "localhost";
	str config_dir = "$HOME/.skivvy";

	for(auto arg = args.begin(); arg != args.end(); ++arg)
	{
		if(*arg == "--config")
		{
			if(++arg == args.end())
			{
				out_con("ERROR: --config requires argument:");
				return 1;
			}
			config_dir = *arg;
		}
		if(*arg == "--host")
		{
			if(++arg == args.end())
			{
				out_con("ERROR: --host requires argument:");
				return 1;
			}
			host = *arg;
		}
		if(*arg == "--port")
		{
			if(++arg == args.end())
			{
				out_con("ERROR: --port requires argument:");
				return 1;
			}
			port = *arg;
		}
	}

	config_dir = wordexp(config_dir);

	SkivvyClient client(config_dir, host, port);

	client.run();
}
