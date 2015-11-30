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

#include "Cookie.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

Cookie::Cookie(Handshake& handshake,Invoker& invoker,const string& tag,const string& queryUrl) : peerId(),_invoker(invoker), _pComputingThread(NULL),_pCookieComputing(new CookieComputing(invoker,&handshake,false)),tag(tag),pTarget(NULL),id(0),farId(0),queryUrl(queryUrl),_writer(_buffer,sizeof(_buffer)) {
	_pComputingThread = invoker.poolThreads.enqueue(_pCookieComputing.cast<WorkThread>(),_pComputingThread);
}

Cookie::Cookie(Handshake& handshake, Invoker& invoker,const string& tag,Target& target) : peerId(),_invoker(invoker),_pComputingThread(NULL),_pCookieComputing(new CookieComputing(invoker,&handshake,true)),tag(tag),pTarget(&target),id(0),farId(0),_writer(_buffer,sizeof(_buffer)) {
	_pCookieComputing->pDH = target.pDH;
}

Cookie::~Cookie() {
}

void Cookie::write() {
	if(_writer.length()==0) {
		_writer.write32(id);
		_writer.write7BitLongValue(_pCookieComputing->nonce.size());
		_writer.writeRaw(&_pCookieComputing->nonce[0],_pCookieComputing->nonce.size());
		_writer.write8(0x58);
	}
}

UInt16 Cookie::read(PacketWriter& writer) {
	writer.writeRaw(_writer.begin(),_writer.length());
	return _writer.length();
}


} // namespace Cumulus
