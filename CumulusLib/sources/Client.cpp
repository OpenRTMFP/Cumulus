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

#include "Client.h"
#include "Util.h"
#include "Logs.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

PacketWriter	BandWriterNull::WriterNull(NULL,0);
BandWriterNull	Client::_BandWriterNull;
FlowWriter		Client::_FlowWriterNull("",_BandWriterNull);

Client::Client() : _pFlowWriter(NULL),_pObject(NULL),ping(0) {
}

Client::~Client() {
}

FlowWriter&	Client::writer() {
	if(!_pFlowWriter)
		WARN("Client::writer() called on %s is null",Util::FormatHex(id,ID_SIZE).c_str());
	return _pFlowWriter ? *_pFlowWriter : _FlowWriterNull;
}

} // namespace Cumulus
