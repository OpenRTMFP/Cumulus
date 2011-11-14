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

namespace Cumulus {

class FlowNull : public Flow {
public:
	FlowNull(Peer& peer,Handler& handler,BandWriter& band);
	virtual ~FlowNull();
	
	void	fragmentHandler(Poco::UInt32 stage,Poco::UInt32 deltaNAck,PacketReader& fragment,Poco::UInt8 flags);

	FlowWriter&			writer();

};

inline FlowWriter& FlowNull::writer() {return Flow::writer;}

} // namespace Cumulus
