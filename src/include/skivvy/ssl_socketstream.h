#pragma once
#ifndef _SKIVVY_SSL_SOCKETSTREAM_H_
#define _SKIVVY_SSL_SOCKETSTREAM_H_

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com               |
'------------------------------------------------------------------'

This code is was created from code (C) Copyright Nicolai M. Josuttis 2001
with the following Copyright Notice:
*/

/* The following code declares classes to read from and write to
 * file descriptore or file handles.
 *
 * See
 *      http://www.josuttis.com/cppcode
 * for details and the latest version.
 *
 * - open:
 *      - integrating BUFSIZ on some systems?
 *      - optimized reading of multiple characters
 *      - stream for reading AND writing
 *      - i18n
 *
 * (C) Copyright Nicolai M. Josuttis 2001.
 * Permission to copy, use, modify, sell and distribute this software
 * is granted provided this copyright notice appears in all copies.
 * This software is provided "as is" without express or implied
 * warranty, and with no claim as to its suitability for any purpose.
 *
 * Version: Jul 28, 2002
 * History:
 *  Jul 28, 2002: bugfix memcpy() => memmove()
 *                fdinbuf::underflow(): cast for return statements
 *  Aug 05, 2001: first public version
 */

 /*

Permission to copy, use, modify, sell and distribute this software
is granted under the same conditions.

'----------------------------------------------------------------*/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <string>
#include <iostream>

typedef std::string str;
typedef size_t siz;

namespace skivvy { namespace net {

struct ssl_connection
{
	int sock;
	SSL *ssl;
	SSL_CTX *ctx;
	ssl_connection(): sock(0), ssl(0), ctx(0) {}
};

template<typename Char>
class basic_ssl_socketbuf
: public std::basic_streambuf<Char>
{
public:
	typedef Char char_type;
	typedef std::basic_streambuf<char_type> buf_type;
	typedef std::basic_ostream<char_type> stream_type;
	typedef typename buf_type::int_type int_type;
	typedef typename std::basic_streambuf<Char>::traits_type traits_type;

protected:
	static const int char_size = sizeof(char_type);
	static const int SIZE = 128;

	ssl_connection conn;
	char_type* ibuf;//[SIZE] ;
	char_type* obuf;//[SIZE] ;


public:
	basic_ssl_socketbuf()
	: conn()
	, ibuf(0)
	, obuf(0)
	{
		ibuf = new char_type[SIZE];
		obuf = new char_type[SIZE];
		buf_type::setp(obuf, obuf + (SIZE - 1));
		buf_type::setg(ibuf, ibuf, ibuf);
	}

	virtual ~basic_ssl_socketbuf()
	{
		sync();
		delete[] ibuf;
		delete[] obuf;
	}

	void set_ssl_connection(const ssl_connection& conn) { this->conn = conn; }
	ssl_connection get_ssl_connection() { return this->conn; }

protected:

	int output_buffer()
	{
		int num = buf_type::pptr() - buf_type::pbase();

		if(SSL_write(conn.ssl, reinterpret_cast<char*>(obuf), num * char_size) != num)
			return traits_type::eof();
		buf_type::pbump(-num);
		return num;
	}

	virtual int_type overflow(int_type c)
	{
		if(c != traits_type::eof())
		{
			*buf_type::pptr() = c;
			buf_type::pbump(1);
		}

		if(output_buffer() == traits_type::eof())
			return traits_type::eof();
		return c;
	}

	virtual int sync()
	{
		if(output_buffer() == traits_type::eof())
			return traits_type::eof();
		return 0;
	}

	virtual int_type underflow()
	{
		if(buf_type::gptr() < buf_type::egptr())
			return *buf_type::gptr();

		int num;
		if((num = SSL_read(conn.ssl, reinterpret_cast<char*>(ibuf), SIZE * char_size)) <= 0)
			return traits_type::eof();

		buf_type::setg(ibuf, ibuf, ibuf + num);
		return *buf_type::gptr();
	}
};

typedef basic_ssl_socketbuf<char> ssl_socketbuf;
typedef basic_ssl_socketbuf<wchar_t> ssl_wsocketbuf;

template<typename Char>
class basic_ssl_socketstream
: public std::basic_iostream<Char>
{
public:
	typedef Char char_type;
	typedef std::basic_iostream<char_type> stream_type;
	typedef basic_ssl_socketbuf<char_type> buf_type;

protected:
	buf_type buf;

public:
	basic_ssl_socketstream(): stream_type(&buf) {}

	virtual ~basic_ssl_socketstream()
	{
		this->close();
	}

	void close()
	{
		if(buf.get_ssl_connection().sock != 0)
			ssl_disconnect();
		stream_type::clear();
	}

	bool open(const std::string& host, uint16_t port)
	{
		close();
		ssl_connection conn;
		if(ssl_connect(host, port, conn))
			buf.set_ssl_connection(ssl_connect(host, port));
		return *this;
	}

	bool tcp_connect(const str& host, siz port, int& sd)
	{
		close();

		if((sd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		{
			log(strerror(errno));
			stream_type::setstate(std::ios::failbit);
			return false;
		}

		hostent* he;
		if(!(he = gethostbyname(host.c_str())))
		{
			::close(sd);
			stream_type::setstate(std::ios::failbit);
			return false;
		}

		sockaddr_in sin;
		std::copy(reinterpret_cast<char*>(he->h_addr)
			, reinterpret_cast<char*>(he->h_addr) + he->h_length
			, reinterpret_cast<char*>(&sin.sin_addr.s_addr));

		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		sin.sin_addr = *((in_addr*) he->h_addr);

		if(connect(sd, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) < 0)
		{
			::close(sd);
			stream_type::setstate(std::ios::failbit);
			return false;
		}

		return true;
	}

//	void print_ERRs()
//	{
//		siz code;
//		while((code = ERR_get_error()) != 0)
//			std::cerr << ERR_error_string(code, 0) << '\n';
//	}
//

	bool ssl_connect(const str& host, siz port, ssl_connection& conn)
	{
		if(!tcp_connect(host, port, conn.sock))
		{
			stream_type::setstate(std::ios::badbit);
			return false;
		}

		SSL_load_error_strings();
		SSL_library_init();

		if(!(conn.ctx = SSL_CTX_new(SSLv23_client_method())))
		{
			stream_type::setstate(std::ios::badbit);
			return false;
		}
		if(!(conn.ssl = SSL_new(conn.ctx)))
		{
			SSL_CTX_free(conn.ctx);
			stream_type::setstate(std::ios::badbit);
			return false;
		}
		if(!SSL_set_fd(conn.ssl, conn.sock))
		{
			SSL_free(conn.ssl);
			SSL_CTX_free(conn.ctx);
			stream_type::setstate(std::ios::badbit);
			return false;
		}
		if(SSL_connect(conn.ssl) != 1)
		{
			SSL_free(conn.ssl);
			SSL_CTX_free(conn.ctx);
			stream_type::setstate(std::ios::badbit);
			return false;
		}

		return true;
	}

	void ssl_disconnect()
	{
		ssl_connection conn = buf.get_ssl_connection();
		if(conn.sock)
			::close(conn.sock);
		if(conn.ssl)
		{
			SSL_shutdown(conn.ssl);
			SSL_free(conn.ssl);
		}
		if(conn.ctx)
			SSL_CTX_free(conn.ctx);
	}

};

typedef basic_ssl_socketstream<char> ssl_socketstream;
typedef basic_ssl_socketstream<wchar_t> ssl_wsocketstream;

}} // skivvy::net

#endif // _SKIVVY_SSL_SOCKETSTREAM_H_
