/* 
	Copyright 2010 cumulus.dev@gmail.com

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

#include "RTMFPServer.h"
#include "Logs.h"

#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/ServerApplication.h"
#include <iostream>

using namespace std;
using namespace Poco;
using namespace Poco::Util;
using namespace Cumulus;

char * g_logPriorities[] = { "FATAL","CRITIC" ,"ERROR","WARN","NOTE","INFO","DEBUG","TRACE" };

class CumulusService: public ServerApplication , private Cumulus::Logger {
public:
	CumulusService(): _helpRequested(false) {
		Logs::SetLogger(*this);
	}
	
	~CumulusService() {
	}

protected:
	void initialize(Application& self) {
		loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
	}
		
	void uninitialize() {
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options) {
		ServerApplication::defineOptions(options);
		
		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false));
	}

	void handleOption(const std::string& name, const std::string& value) {
		ServerApplication::handleOption(name, value);

		if (name == "help")
			_helpRequested = true;
	}

	void displayHelp() {
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("Cumulus 'rendezvous' server for RTMFP communication");
		helpFormatter.format(cout);
	}

	void logHandler(Thread::TID threadId,const std::string& threadName,Priority priority,const char *filePath,long line, const char *text) {
		printf("%s  %s[%ld] %s\n",g_logPriorities[priority-1],Path(filePath).getBaseName().c_str(),line,text);
	}

	int main(const std::vector<std::string>& args) {
		if (_helpRequested) {
			displayHelp();
		}
		else {
			RTMFPServer server;
			server.start(config().getInt("port", 1935),"rtmfp://216.104.221.7:10007/9292ca89f91b35a5425c44f0-8a269df2193f");
			//cumulus.start(config().getInt("port", 1935));
			// wait for CTRL-C or kill
			waitForTerminationRequest();
			// Stop the HTTPServer
			server.stop();
		}
		return Application::EXIT_OK;
	}
	
private:
	bool _helpRequested;
};




int main(int argc, char* argv[]) {
	return CumulusService().run(argc, argv);
}