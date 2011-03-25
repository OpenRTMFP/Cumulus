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
#include "AMFWriter.h"
#include "AMFObjectWriter.h"

namespace Cumulus {


class ResponseWriter {
public:
	ResponseWriter(PacketWriter& writer,double callbackHandle,const std::string& flowName,const std::string& msgName);
	virtual ~ResponseWriter();

	AMFWriter&		writeAMFResponse();
	PacketWriter&	writeRawResponse(bool withoutHeader=false);
	AMFObjectWriter	writeSuccessResponse(const std::string& description,const std::string& name="Success");
	AMFObjectWriter	writeErrorResponse(const std::string& error,const std::string& name="Failed");

	Poco::UInt8		count();
	void			flush();

private:
	PacketWriter	_rawWriter;
	AMFWriter		_amfWriter;
	double			_callbackHandle;
	std::string		_code;

	int					_beginMsg;
	int					_beginCnt;
	Poco::UInt8			_count;
};

inline Poco::UInt8 ResponseWriter::count() {
	return _count;
}

} // namespace Cumulus
