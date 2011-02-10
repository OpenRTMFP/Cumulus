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

#include "AMFResponse.h"

using namespace std;

namespace Cumulus {

AMFResponse::AMFResponse(AMFWriter& writer,double responderHandle,const string& error) : AMFWriter(writer) {
	if(!error.empty()) {
		writer.write("_error");
		writer.writeNumber(responderHandle);
		writer.writeNull();
		writer.beginObject();
		writer.writeObjectProperty("level","error");
		writer.writeObjectProperty("code","NetConnection.Call.Failed");
		writer.writeObjectProperty("description",error);
		writer.endObject();
	} else {
		writer.write("_result");
		writer.writeNumber(responderHandle);
		writer.writeNull();
	}
}


AMFResponse::~AMFResponse() {

}


} // namespace Cumulus
