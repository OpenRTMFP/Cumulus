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

#include "Clients.h"
#include "Util.h"
#include "Logs.h"

using namespace std;
using namespace Poco;

namespace Cumulus {

PacketWriter	BandWriterNull::WriterNull(NULL,0);
BandWriterNull	Client::_BandWriterNull;
FlowWriter		Client::_FlowWriterNull("",_BandWriterNull);

Client::Client() : _pFlowWriter(NULL) {
}

Client::Client(const Client& client) : Entity(client),path(client.path),swfUrl(client.swfUrl),pageUrl(client.pageUrl),_pFlowWriter(client._pFlowWriter) {
	copyProperties(*this);
}

Client::~Client() {
}

void Client::copyProperties(const AbstractConfiguration& abstractConfigs,const string& root) {
	AbstractConfiguration::Keys keys;
	abstractConfigs.keys(root,keys);
	AbstractConfiguration::Keys::const_iterator it;
	for(it=keys.begin();it!=keys.end();++it) {
		string key(root);
		if(!key.empty())
			key+=".";
		key += (*it);
		if(abstractConfigs.hasOption(key))
			setString(key,abstractConfigs.getString(key));
		else
			copyProperties(abstractConfigs,key);
	}
}

FlowWriter&	Client::writer() {
	if(!_pFlowWriter)
		WARN("Client::writer() called on %s is null",Util::FormatHex(id,ID_SIZE).c_str());
	return _pFlowWriter ? *_pFlowWriter : _FlowWriterNull;
}

Clients::Clients(map<const UInt8*,Client*,Compare>& clients) : _clients(clients) {
}

Clients::~Clients() {
}

Client* Clients::operator()(const UInt8* id) {
	Iterator it = _clients.find(id);
	if(it==_clients.end())
		return NULL;
	return it->second;
}

} // namespace Cumulus
