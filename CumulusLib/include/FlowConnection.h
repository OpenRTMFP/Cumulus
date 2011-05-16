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

#include "Cumulus.h"
#include "Flow.h"
#include "AMFReader.h"
#include "AMFWriter.h"

namespace Cumulus {

class FlowConnection : public Flow {
public:
	FlowConnection(Poco::UInt8 id,Peer& peer,ServerHandler& serverHandler,BandWriter& band);
	virtual ~FlowConnection();

	void complete();

	static std::string	s_signature;

private:
	static std::string	s_name;
	void	messageHandler(const std::string& name,AMFReader& message);

	std::set<Poco::UInt32> _streamIndex;
};

} // namespace Cumulus
