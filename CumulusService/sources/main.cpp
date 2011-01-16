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
#include "Auth.h"
#include "Poco/StringTokenizer.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/ServerApplication.h"
#include <iostream>

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;
using namespace Cumulus;

char * g_logPriorities[] = { "FATAL","CRITIC" ,"ERROR","WARN","NOTE","INFO","DEBUG","TRACE" };
// TODO on linux : warning: deprecated conversion from string constant to ‘char*’


class CumulusService: public ServerApplication , private Cumulus::Logger, private Cumulus::ClientHandler {
public:
	CumulusService(): _helpRequested(false),_pCirrus(NULL) {
		Logs::SetLogger(*this);
	}
	
	~CumulusService() {
		if(_pCirrus)
			delete _pCirrus;
	}

protected:
	void initialize(Application& self) {
		loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
		_auth.authIsWhitelist=config().getBool("auth.whitelist",false);
		_auth.load("auth");
	}
		
	void uninitialize() {
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options) {
		ServerApplication::defineOptions(options);

		options.addOption(
			Option("cirrus", "c", "Cirrus address to activate a 'man-in-the-middle' developer mode in bypassing flash packets to the official cirrus server of your choice, it's a instable mode to help Cumulus developers. You may add (after a comma) an option to include middle packets process for your dumping (only with /dump).\nExample: 'p2p.rtmfp.net:10007,1'.",false,"address[,dump]",true)
				.repeatable(false));

		options.addOption(
			Option("dump", "d", "Enables packet traces in the console. Optionnal 'file' argument also allows a file dumping. Used often with 'cirrus=address[,dump]' option to observe flash/cirrus exchange.",false,"file",false)
				.repeatable(false));

		options.addOption(
			Option("log", "l", "Log level argument, must be beetween 0 and 8 : nothing, fatal, critic, error, warn, note, info, debug, trace. Default value is 6 (note), all logs until info level are displayed.")
				.required(false)
				.argument("level")
				.repeatable(false));

		options.addOption(
			Option("help", "h", "Displays help information about command-line usage.")
				.required(false)
				.repeatable(false));
	}

	void handleOption(const std::string& name, const std::string& value) {
		ServerApplication::handleOption(name, value);

		if (name == "help")
			_helpRequested = true;
		else if (name == "cirrus") {
			string address;
			StringTokenizer split(value,",",StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
			if(split.count()>0)
				address = split[0];
			if(split.count()>1 && split[1]!="0")
				Logs::Middle(true);
			try {
				if(address.empty())
					throw Exception("cirrus address must be indicated");
				if(_pCirrus)
					delete _pCirrus;
				URI uri("rtmfp://"+address);
				_pCirrus = new SocketAddress(uri.getHost(),uri.getPort());
				INFO("Mode 'man in the middle' : the exchange will bypass to '%s'",address.c_str());
			} catch(Exception& ex) {
				ERROR("Mode 'man in the middle' error : %s",ex.displayText().c_str());
			}
		} else if (name == "dump")
			Logs::Dump(true,value);
		else if (name == "middle")
			Logs::Middle(true);
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
		printf("%s  %s[%ld] %s",g_logPriorities[priority-1],Path(filePath).getBaseName().c_str(),line,text);
		cout << endl;
	}

	bool onConnection(Client& client) {
		// Here you can read custom client http parameters in reading "client.parameters".
		// Also you can send custom data for the client in writing in "client.data",
		// on flash side you could read that on "data" property from NetStatusEvent::NET_STATUS event of NetConnection object
		return _auth.check(client); //Acceptance
	}
	void onFailed(const Client& client,const std::string& msg) {
		
	}
	void onDisconnection(const Client& client) {

	}

	int main(const std::vector<std::string>& args) {
		if (_helpRequested) {
			displayHelp();
		}
		else {
			/// Cumulus Service
			RTMFPServer server(*this,config().getInt("keepAliveServer",15),config().getInt("keepAlivePeer",10));
			server.start(config().getInt("port", RTMFP_DEFAULT_PORT),_pCirrus);
			// wait for CTRL-C or kill
			waitForTerminationRequest();
			// Stop the HTTPServer
			server.stop();
		}
		return Application::EXIT_OK;
	}
	
private:
	bool			_helpRequested;
	SocketAddress*	_pCirrus;
	Auth			_auth;
};


int main(int argc, char* argv[]) {
	return CumulusService().run(argc, argv);
}