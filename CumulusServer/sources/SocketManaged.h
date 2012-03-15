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

#include "Poco/Net/Socket.h"

class SocketManaged {
	friend class SocketManager;
protected:
	SocketManaged(const Poco::Net::Socket& socket):error(false),writable(false),readable(false),socket(socket){}
	virtual ~SocketManaged(){}
	
private:
	virtual void	onReadable(Poco::UInt32 available)=0;
	virtual void	onWritable()=0;
	virtual void	onError(const std::string& error)=0;

	virtual bool	haveToWrite()=0;

	const bool					error;
	const bool					writable;
	const bool					readable;

	const Poco::Net::Socket&	socket;
};
