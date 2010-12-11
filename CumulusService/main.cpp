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

#include "RTMFPServer.h"
#include "Logs.h"
#include "Database.h"

#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/ServerApplication.h"
#include <iostream>

using namespace std;
using namespace Poco;
using namespace Poco::Data;
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
			Option("help", "h", "displays help information about command-line usage")
				.required(false)
				.repeatable(false));

		options.addOption(
			Option("cirrus", "c", "cirrus url to activate a 'man-in-the-middle' mode in bypassing flash packets to the official cirrus server of your choice")
				.required(false)
				.repeatable(false));

		options.addOption(
			Option("dump", "d", "enables packet traces in the console. Optionnal 'file' argument also allows a file dumping. Often used with 'cirrus=url' option to observe flash/cirrus exchange")
				.required(false)
				.argument("file")
				.repeatable(false));

		options.addOption(
			Option("log", "l", "log level argument, must be beetween 0 and 8 : nothing, fatal, critic, error, warn, note, info, debug, trace")
				.required(false)
				.argument("level")
				.repeatable(false));
	}

	void handleOption(const std::string& name, const std::string& value) {
		ServerApplication::handleOption(name, value);

		if (name == "help")
			_helpRequested = true;
		else if (name == "cirrus")
			_cirrusUrl = value;
		else if (name == "dump")
			Logs::Dump(true,value);
		else if (name == "log")
			Logs::SetLevel(atoi(value.c_str()));
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
			Database::Load(
				config().getString("database.connector",SQLite::Connector::KEY),
				config().getString("database.connectionString","data.db"));
				
			RTMFPServer server;
			server.start(config().getInt("port", CUMULUS_DEFAULT_PORT),_cirrusUrl);
			// wait for CTRL-C or kill
			waitForTerminationRequest();
			// Stop the HTTPServer
			server.stop();

			Database::Unload();
		}
		return Application::EXIT_OK;
	}
	
private:
	bool	_helpRequested;
	string	_cirrusUrl;
};




int main(int argc, char* argv[]) {
	return CumulusService().run(argc, argv);
}