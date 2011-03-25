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

#include "ResponseWriter.h"
#include "Logs.h"

using namespace std;

namespace Cumulus {

ResponseWriter::ResponseWriter(PacketWriter& writer,double callbackHandle,const string& flowName,const string& msgName) : _code(flowName),_rawWriter(writer),_amfWriter(_rawWriter),_callbackHandle(callbackHandle),_beginMsg(writer.position()),_count(0),_beginCnt(writer.length()) {
	if(!msgName.empty()) {
		_code.append(".");
		_code.append(msgName);
	}
	_rawWriter.reset(_beginCnt);
}

ResponseWriter::~ResponseWriter() {
	_rawWriter.clear(_beginMsg);
}

PacketWriter& ResponseWriter::writeRawResponse(bool withoutHeader) {
	flush();
	if(!withoutHeader) {
		_rawWriter.write8(0x04);
		_rawWriter.write32(0);
	}
	return _rawWriter;
}

AMFWriter& ResponseWriter::writeAMFResponse() {
	flush();
	_rawWriter.write8(0x14);
	_rawWriter.write32(0);
	_amfWriter.write("_result");
	_amfWriter.writeNumber(_callbackHandle);
	_amfWriter.writeNull();
	return _amfWriter;
}

AMFObjectWriter ResponseWriter::writeSuccessResponse(const string& description,const string& name) {
	writeAMFResponse();

	string code(_code);
	code.append(".");
	code.append(name);

	AMFObjectWriter object(_amfWriter);
	object.write("level","status");
	object.write("code",code);
	if(!description.empty())
		object.write("description",description);
	return object;
}

AMFObjectWriter ResponseWriter::writeErrorResponse(const string& description,const string& name) {
	flush();
	_rawWriter.write8(0x14);
	_rawWriter.write32(0);
	_amfWriter.write("_error");
	_amfWriter.writeNumber(_callbackHandle);
	_amfWriter.writeNull();

	string code(_code);
	code.append(".");
	code.append(name);

	AMFObjectWriter object(_amfWriter);
	object.write("level","error");
	object.write("code",code);
	if(!description.empty())
		object.write("description",description);

	WARN("'%s' response error : %s",code.c_str(),description.c_str());
	return object;
}

void ResponseWriter::flush() {
	if(_rawWriter.length()>_beginCnt) {
		_rawWriter.reset(_beginMsg);
		_rawWriter.write8(_count ? 0x11 : 0x10);
		++_count;
		int len = _rawWriter.length()-_rawWriter.position()-2;
		if(len<0)
			len = 0;
		_rawWriter.write16(len);
		_rawWriter.next(len+3);
		_beginCnt=_rawWriter.position();
		_beginMsg=_beginCnt-3;
	}
}

} // namespace Cumulus
