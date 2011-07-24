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
#include "PacketWriter.h"

#define SYMETRIC_ENCODING	0x01
#define WITHOUT_ECHO_TIME   0x02

namespace Cumulus {

class FlowWriter;
class BandWriter {
public:
	BandWriter() {}
	virtual ~BandWriter() {}

	virtual void			initFlowWriter(FlowWriter& flowWriter)=0;
	virtual void			resetFlowWriter(FlowWriter& flowWriter)=0;

	virtual bool			canWriteFollowing(FlowWriter& flowWriter)=0;
	virtual PacketWriter&	writer()=0;
	virtual PacketWriter&	writeMessage(Poco::UInt8 type,Poco::UInt16 length,FlowWriter* pFlowWriter=NULL)=0;
	virtual void			flush(Poco::UInt8 flags=0)=0;
	
};


} // namespace Cumulus
