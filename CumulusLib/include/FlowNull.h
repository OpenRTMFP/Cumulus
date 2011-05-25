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
	FlowNull(Peer& peer,ServerHandler& serverHandler,BandWriter& band);
	virtual ~FlowNull();
	
	void	fragmentHandler(Poco::UInt32 stage,Poco::UInt32 deltaNAck,PacketReader& fragment,Poco::UInt8 flags);

	void				complete();
	FlowWriter&			writer();

	
private:	
	void				commit();

	static std::string	s_signature;
	static std::string	s_name;
};

inline void FlowNull::complete() {} // To overload the Flow definition, to avoid to set '_completed' for 'true', FlowNull must not be deleted!

inline FlowWriter& FlowNull::writer() {return Flow::writer;}

} // namespace Cumulus
