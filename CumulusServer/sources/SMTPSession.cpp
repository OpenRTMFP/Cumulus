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

#include "SMTPSession.h"
#include "Logs.h"
#include "Poco/NumberFormatter.h"
#include "Poco/Net/StringPartSource.h"
#include "Poco/Net/NetException.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;


SMTPSession::SMTPSession(const string& host,UInt16 port,UInt16 timeout) : _SMTPClient(_socket),Startable("SMTPSession"),_host(host),_port(port),_timeout(timeout*1000),_opened(false) {
	setPriority(Thread::PRIO_LOWEST);
}


SMTPSession::~SMTPSession() {
	{
		ScopedLock<FastMutex> lock(_mutexError);
		_error = "STMPSession deleting";
		ScopedLock<FastMutex> lock2(_mutex);
		_opened=false;
	}
	_socket.close();
	_mailEvent.set();
	stop();
	list<MailMessage*>::const_iterator it;
	for(it=_mails.begin();it!=_mails.end();++it) {
		onSent();
		delete *it;
	}
}

const char* SMTPSession::error() {
	ScopedLock<FastMutex> lock(_mutexError);
	return _error.empty() ? NULL : _error.c_str();
}

void SMTPSession::send(const string& sender,const string& recipient,const string& subject,const string& content) {
	list<string> recipients;
	recipients.push_back(recipient);
	send(sender,recipients,subject,content);
}

void SMTPSession::send(const string& sender,const list<string>& recipients,const string& subject,const string& content) {
	ScopedLock<FastMutex> lock(_mutex);
	if(!_opened) {
		_opened=true;
		stop();
		start();
	}
	MailMessage* pMail = new MailMessage();
	pMail->setSender(sender);
	list<string>::const_iterator it;
	for(it=recipients.begin();it!=recipients.end();++it) {
		pMail->addRecipient(MailRecipient(MailRecipient::PRIMARY_RECIPIENT, *it));
	}
	pMail->setSubject(subject);
	pMail->setContent(content);
	_mails.push_back(pMail);
	_mailEvent.set();
}


void SMTPSession::run(const volatile bool& terminate) {
	{
		ScopedLock<FastMutex> lock(_mutexError);
		_error.clear();
	}

	try {
		_socket.connect(SocketAddress(_host,_port),_timeout);
		_socket.setReceiveTimeout(_timeout);
		_socket.setSendTimeout(_timeout);
		_SMTPClient.login();
	} catch(NetException& ex) {
		ScopedLock<FastMutex> lock(_mutexError);
		_error = ex.displayText() + " (check " + _host + ":" + NumberFormatter::format(_port) + " SMTP server configurations)";
	} catch(Exception& ex) {
		ScopedLock<FastMutex> lock(_mutexError);
		_error = ex.displayText();
	}


	{
		ScopedLock<FastMutex> lock(_mutexError);
		if(!_error.empty()) {
			ScopedLock<FastMutex> lock(_mutex);
			_opened=false;
			list<MailMessage*>::const_iterator it;
			for(it=_mails.begin();it!=_mails.end();++it) {
				onSent();
				delete *it;
			}
			return;
		}
	}

	while(!terminate) {

		MailMessage* pMail;
		{
			ScopedLock<FastMutex> lock(_mutex);
			if(_mails.empty()) {
				_mutex.unlock();
				_mailEvent.tryWait(_timeout);
				_mutex.lock();
				if(_mails.empty()) {
					// timeout
					try {
						_SMTPClient.close();
					} catch(Exception& ex) {
						ScopedLock<FastMutex> lock(_mutexError);
						if(_error.empty())
							_error = ex.displayText();
					}
					_opened=false;
					break;
				}
			}
			if(!_opened)
				break;
			pMail = _mails.front();
			_mails.pop_front();
		}


		try {
			_SMTPClient.sendMessage(*pMail);
		} catch(Exception& ex) {
			ScopedLock<FastMutex> lock(_mutexError);
			if(_error.empty())
				_error = ex.displayText();
		}
		onSent();
		delete pMail;
	}
	
}
