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
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/ServerApplication.h"
#include <iostream>

#define LOG_FILE(END)	"./logs/log."#END

#define LOG_SIZE 1000000

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::Util;
using namespace Cumulus;

char * g_logPriorities[] = { "FATAL","CRITIC" ,"ERROR","WARN","NOTE","INFO","DEBUG","TRACE" };
// TODO on linux : warning: deprecated conversion from string constant to ‘char*’


class CumulusService: public ServerApplication , private Cumulus::Logger, private Cumulus::ClientHandler {
public:
	CumulusService(): _helpRequested(false),_pCirrus(NULL),_logFile(LOG_FILE(0)) {
		File("./logs").createDirectory();
		_logStream.open(LOG_FILE(0),ios::in | ios::ate);
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
			Option("cirrus", "c", "Cirrus address to activate a 'man-in-the-middle' developer mode in bypassing flash packets to the official cirrus server of your choice, it's a instable mode to help Cumulus developers, \"p2p.rtmfp.net:10007\" for example. Adding the 'dump' option in 'all' mode displays the middle packet process in your logs (see 'dump' argument).",false,"address",true)
				.repeatable(false));

		options.addOption(
			Option("dump", "d", "Enables packet traces in logs. Used usually with 'cirrus=address' option to observe flash/cirrus exchange. Optional argument 'all' displays all packet process like middle packet process in 'man-in-the-middle' mode (see 'cirrus=address' argument).",false,"all",false)
				.repeatable(false));

		options.addOption(
			Option("log", "l", "Log level argument, must be beetween 0 and 8 : nothing, fatal, critic, error, warn, note, info, debug, trace. Default value is 6 (info), all logs until info level are displayed.")
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
			try {
				if(_pCirrus)
					delete _pCirrus;
				URI uri("rtmfp://"+value);
				_pCirrus = new SocketAddress(uri.getHost(),uri.getPort());
				INFO("Mode 'man in the middle' : the exchange will bypass to '%s'",value.c_str());
			} catch(Exception& ex) {
				ERROR("Mode 'man in the middle' error : %s",ex.displayText().c_str());
			}
		} else if (name == "dump")
			Logs::EnableDump(value == "all");
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

	void dumpHandler(const char* data,int size) {
		cout.write(data,size);
		_logStream.write(data,size);
		manageLogFile();
	}

	void logHandler(Thread::TID threadId,const std::string& threadName,Priority priority,const char *filePath,long line, const char *text) {
		printf("%s  %s[%ld] %s\n",g_logPriorities[priority-1],Path(filePath).getBaseName().c_str(),line,text);
		_logStream << DateTimeFormatter::format(LocalDateTime(),"%d/%m %H:%M:%S.%c  ")
				<< g_logPriorities[priority-1] << '\t' << threadName << '(' << threadId << ")\t"
				<< Path(filePath).getFileName() << '[' << line << "]  " << text << std::endl;
		manageLogFile();
	}

	void manageLogFile() {
		if(_logFile.getSize()>LOG_SIZE) {
			_logStream.close();
			int num = 10;
			File file(LOG_FILE(10));
			if(file.exists())
				file.remove();
			while(--num>=0) {
				file = LOG_FILE(num);
				if(file.exists())
					file.renameTo(LOG_FILE(num+1));
			}
			_logStream.open(LOG_FILE(0),ios::in | ios::ate);
		}	
	}

	bool onConnection(Client& client) {

		//Acceptance
		if(!_auth.check(client))
			return false;

		// Here you can read custom client http parameters in reading "client.parameters".
		// Also you can send custom data for the client in writing in "client.data",
		// on flash side you could read that on "data" property from NetStatusEvent::NET_STATUS event of NetConnection object

		// Implementation of families!
		map<string,string>::const_iterator it = client.parameters.find("family");
		if(it!=client.parameters.end()) {
			set<const Client*>& clients = _clientFamilies[it->second];
			// return peer Ids includes in this family
			if(clients.size()>2000)
				return false; // 2000 members in a family is the maximum possible!
			set<const Client*>::const_iterator it;
			client.data.resize(clients.size()*32);
			int i=0;
			for(it=clients.begin();it!=clients.end();++it) {
				memcpy(&client.data[i],(*it)->id,32);
				i += 32;
			}
			clients.insert(&client);
		}
		return true;
	}
	void onFailed(const Client& client,const string& msg) {
		ERROR(msg.c_str());
	}
	void onDisconnection(const Client& client) {
		map<string,string>::const_iterator it = client.parameters.find("family");
		if(it!=client.parameters.end())
			_clientFamilies[it->second].erase(&client);
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
	File			 _logFile;
	FileOutputStream _logStream;
	map<string,set<const Client*>> _clientFamilies;	
};


int main(int argc, char* argv[]) {
	return CumulusService().run(argc, argv);
}