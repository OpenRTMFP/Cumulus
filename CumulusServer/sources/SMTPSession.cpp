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
using namespace Cumulus;
using namespace Poco;
using namespace Poco::Net;


class Mail : public MailMessage {
public:
	Mail(MailHandler* pHandler) : pHandler(pHandler) {
	}
	virtual ~Mail() {
		if(pHandler)
			pHandler->onSent(error.empty() ? NULL : error.c_str());
	}
	string		 error;
	MailHandler* pHandler;
};


SMTPSession::SMTPSession(TaskHandler& handler,const string& host,UInt16 port,UInt16 timeout) : _SMTPClient(_socket),Startable("SMTPSession"),Task(handler),_host(host),_port(port),_timeout(timeout*1000) {
	
}


SMTPSession::~SMTPSession() {
	clear();
}

void SMTPSession::clear() {
	{
		ScopedLock<FastMutex> lock(_mutexError);
		_error = "STMPSession clearing";
	}
	_socket.close();
	stop();
	list<Mail*>::const_iterator it;
	for(it=_mailsSent.begin();it!=_mailsSent.end();++it)
		delete *it;
	_mailsSent.clear();
	for(it=_mails.begin();it!=_mails.end();++it) {
		(*it)->error = _error;
		delete *it;
	}
	_mails.clear();
}

void SMTPSession::handle() {
	list<Mail*>::const_iterator it;
	for(it=_mailsSent.begin();it!=_mailsSent.end();++it)
		delete *it;
	_mailsSent.clear();
}

const char* SMTPSession::lastError() {
	ScopedLock<FastMutex> lock(_mutexError);
	return _error.empty() ? NULL : _error.c_str();
}

void SMTPSession::send(const string& sender,const string& recipient,const string& subject,const string& content,MailHandler* pMailHandler) {
	list<string> recipients;
	recipients.push_back(recipient);
	send(sender,recipients,subject,content,pMailHandler);
}

void SMTPSession::send(const string& sender,const list<string>& recipients,const string& subject,const string& content,MailHandler* pMailHandler) {
	ScopedLock<FastMutex> lock(_mutex);
	if(!running())
		start();
	Mail* pMail = new Mail(pMailHandler);
	pMail->setSender(sender);
	list<string>::const_iterator it;
	for(it=recipients.begin();it!=recipients.end();++it) {
		pMail->addRecipient(MailRecipient(MailRecipient::PRIMARY_RECIPIENT, *it));
	}
	pMail->setSubject(subject);
	pMail->setContent(content);
	_mails.push_back(pMail);
	wakeUp();
}


void SMTPSession::run() {
	setPriority(Thread::PRIO_LOWEST);

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
			list<Mail*>::const_iterator it;
			for(it=_mails.begin();it!=_mails.end();++it) {
				Mail* pMail = *it;
				if(pMail->pHandler) {
					pMail->error = _error;
					_mailsSent.push_back(pMail);
				} else
					delete pMail;
			}
			_mails.clear();
			return;
		}
	}

	if(_mailsSent.size()>0)
		waitHandle();

	while(sleep(_timeout)!=STOP) {

		Mail* pMail;
		{
			ScopedLock<FastMutex> lock(_mutex);
			if(_mails.empty()) {
				// timeout
				try {
					_SMTPClient.close();
				} catch(Exception& ex) {
					ScopedLock<FastMutex> lock(_mutexError);
					if(_error.empty())
						_error = ex.displayText();
				}
				stop();
				break;
			}
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

		if(!pMail->pHandler) {
			delete pMail;
		} else {
			pMail->error = _error;
			_mailsSent.push_back(pMail);
			waitHandle();
		}
	}
	
}
