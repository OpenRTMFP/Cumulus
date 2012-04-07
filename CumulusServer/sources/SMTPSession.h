/* 
	Copyright 2010 OpenRTMFP
 
	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License received along this program for more
	details (or else see http://www.gnu.org/licenses/).

	This file is a part of Cumulus.
*/

#pragma once

#include "Startable.h"
#include "MailHandler.h"
#include "Poco/Event.h"
#include "Poco/Net/SMTPClientSession.h"
#include "Poco/Net/MailMessage.h"
#include <list>


class Mail;
class SMTPSession : private Cumulus::Startable {
public:
	enum
	{
		SMTP_PORT = 25
	};


	SMTPSession(const std::string& host,Poco::UInt16 port = SMTP_PORT,Poco::UInt16 timeout=60);
	virtual ~SMTPSession();

	const char*		lastError();
	void			fill();
	void			clear();

	void			send(const std::string& sender,const std::string& recipient,const std::string& subject,const std::string& content,MailHandler* pMailHandler=NULL);
	void			send(const std::string& sender,const std::list<std::string>& recipients,const std::string& subject,const std::string& content,MailHandler* pMailHandler=NULL);

private:
	void			run();


	Poco::FastMutex							_mutex;
	Poco::FastMutex							_mutexSent;
	std::list<Mail*>						_mails;
	std::list<Mail*>						_mailsSent;

	Poco::FastMutex							_mutexError;
	std::string								_error;
	
	Poco::Net::StreamSocket					_socket;
	Poco::Net::SMTPClientSession			_SMTPClient;

	Poco::UInt32							_timeout;
	std::string								_host;
	Poco::UInt16							_port;
	Poco::Event								_mailEvent;
};