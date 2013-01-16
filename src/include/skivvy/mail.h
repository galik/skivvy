/*
 * mail.h
 *
 *  Created on: Jan 16, 2013
 *      Author: oaskivvy@googlemail.com
 */

#ifndef MAIL_H_
#define MAIL_H_

#include <skivvy/types.h>
#include <skivvy/logrep.h>
#include <skivvy/socketstream.h>

namespace skivvy { namespace email {

using namespace skivvy;
using namespace skivvy::types;

class SMTP
{
	const str host;
	const long port;

public:
	// protocol
	str mailfrom;
	str rcptto;

	// headers
	str from;
	str to;
	str replyto;

	SMTP(const str& host, long port = 25): host(host), port(port) {}

	bool sendmail(const str& subject, const str& data)
	{
		net::socketstream ss;

		str reply;

		std::time_t t = std::time(0);
		str time = std::ctime(&t);
		trim(time);

		ss.open(host, port);

		if(!sgl(ss, reply))
		{
			log(strerror(errno));
			return false;
		}
		log(trim(reply));
		if(reply.find("220"))
			return false;

		ss << "HELO sookee.dyndns.org" << "\r\n" << std::flush;
		if(!sgl(ss, reply))
		{
			log(strerror(errno));
			return false;
		}
		log(trim(reply));
		if(reply.find("250"))
			return false;

		// one or more
		{
			ss << "MAIL FROM: " << mailfrom << "\r\n" << std::flush;
			if(!sgl(ss, reply))
			{
				log(strerror(errno));
				return false;
			}
			log(trim(reply));
			if(reply.find("250"))
				return false;

			// one or more
			{
				ss << "RCPT TO: " << rcptto << "\r\n" << std::flush;
				if(!sgl(ss, reply))
				{
					log(strerror(errno));
					return false;
				}
				log(trim(reply));
				if(reply.find("250"))
					return false;
			}

			ss << "DATA" << "\r\n" << std::flush;
			ss << "From: " << mailfrom << "\r\n" << std::flush;
			ss << "To: " << rcptto << "\r\n" << std::flush;
			ss << "Date: " << time << "\r\n" << std::flush;
			ss << "Subject: " << subject << "\r\n" << std::flush;
			ss << "Reply-To: " << replyto << "\r\n" << std::flush;
			ss << "\r\n" << std::flush;

			ss << data << "\r\n" << std::flush;
			ss << "." << "\r\n" << std::flush;
			if(!sgl(ss, reply))
			{
				log(strerror(errno));
				return false;
			}
			log(trim(reply));
			if(reply.find("354"))
				return false;
		}

		ss << "QUIT" << "\r\n" << std::flush;
		if(!sgl(ss, reply))
		{
			log(strerror(errno));
			return false;
		}
		log(trim(reply));
		if(reply.find("250"))
			return false;

		ss.close();

		return true;
	}
};

//int main()
//{
//	SMTP smtp("localhost", 25);
//
//	smtp.mailfrom = "<noreply@sookee.dyndns.org>";
//	smtp.rcptto = "<oasookee@gmail.com>";
//
//	smtp.from = "No Reply " + smtp.mailfrom;
//	smtp.to = "SooKee " + smtp.rcptto;
//	smtp.replyto = "Skivvy <oaskivvy@gmail.com>";
//
//	smtp.sendmail("Another hopeless go three.", "Time to be different again and again.");
//}


}} // skivvy::email


#endif /* MAIL_H_ */
