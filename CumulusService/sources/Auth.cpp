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

#include "Auth.h"
#include "Poco/StringTokenizer.h"
#include "Poco/String.h"
#include "Poco/File.h"
#include "Poco/Path.h"
#include <fstream>

using namespace std;
using namespace Poco;
using namespace Cumulus;


class Line {
public:
	Line(const string& host,const string& path) : _host(trim(host)),_swf(Path(path).getExtension()=="swf"),_path(trim(path)) {
	}

	bool match(const Client& client) {
		if(icompare(client.pageUrl.getHost(),_host)==0) {
			if(_path.empty())
				return true;
			string path(_swf ? client.swfUrl.getPath() : client.pageUrl.getPath());
			if(icompare(path,_path)==0)
				return true;
		}
		return false;
	}
private:
	string	_path;
	string	_host;
	bool	_swf;
};

Auth::Auth() : authIsWhitelist(false) {
}


Auth::~Auth() {
	/// Clear _terms
	vector<Line*>::const_iterator it;
	for(it=_lines.begin();it!=_lines.end();++it)
		delete *it;
	_lines.clear();
}

void Auth::load(const string& file) {
	/// Read auth file
	File f(file);
	if(f.exists() && f.isFile()) {
		ifstream  istr(file.c_str());
		string line;
		while(getline(istr,line)) {
			trimInPlace(line);
			// remove comments
			size_t pos = line.find('#');
			if(pos!=string::npos)
				line = line.substr(0,pos);
			
			if(!line.empty()) {
				StringTokenizer terms(line,",",StringTokenizer::TOK_IGNORE_EMPTY|StringTokenizer::TOK_TRIM);
				if(terms.count()>0) {
					string host(terms[0]),path;
					if(terms.count()>1)
						path.assign(terms[1]);
					_lines.push_back(new Line(host,path));
				}
			}
		}
	}
}

bool Auth::check(const Client& client) {
	vector<Line*>::const_iterator it;
	for(it=_lines.begin();it!=_lines.end();++it) {
		if((*it)->match(client))
			return authIsWhitelist;
	}
	return !authIsWhitelist;
}
