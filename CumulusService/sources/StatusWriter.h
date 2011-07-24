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

#include "FlowWriter.h"
#include "Publication.h"

class StatusWriter : public Cumulus::FlowWriter {
public:
	StatusWriter(const std::string& signature,Cumulus::BandWriter& band);
	virtual ~StatusWriter();

	void addPublication(const Cumulus::Publication& publication);
	void removePublication(const Cumulus::Publication& publication);
	void audioLostRate(const Cumulus::Publication& publication);
	void videoLostRate(const Cumulus::Publication& publication);
	
	void init(Cumulus::Handler& handler);

	Poco::UInt32 idPublisherWithListeners;

private:
	void refresh(Cumulus::Handler& handler);
	void manage(Cumulus::Handler& handler);
	void writePublicationContent(const Cumulus::Publication& publication,Cumulus::AMFWriter& writer);

	std::map<Poco::UInt32,const Cumulus::Publication* const>	_publications;
};

