
Cumulus
=======================================

Cumulus is a crossplatform "rendezvous" service to assist P2P connection in RTMFP peer's communication. In RTMFP protocol, Cumulus plays the server role.
Cumulus can be used as a library to include in your "publisher" software, or be installed as a service on your server computer.

We remind you that Cumulus is licensed under the [GNU General Public License], so **CumulusLib can't be linked with any closed source project** (see [license]).

Status
------------------------------------
Cumulus is in development, much work remains to be done.
If you are a developer **help us** to evolve and enhance Cumulus, else you can always make a **donation** ([us]|[eu]) for that we spent more time on it, in fact it's not technical skills that hinder us but lack of time.

Usage
------------------------------------

### CumulusService

ComulusService is statically configured by a optionnal configuration file to the installation directory.
The possible configurations are:

- **port**,
equals 1935 by default (RTMFP server default port), it's the port used by CumulusService to listen incoming RTMFP requests.

- **keepAliveServer**,
time in seconds for periodically sending packets keep-alive with server, 15s by default (valid value is from 5s to 255s).

- **keepAlivePeer**,
time in seconds for periodically sending packets keep-alive between peers, 10s by default (valid value is from 5s to 255s).

- **auth.whitelist**,
boolean value to interpret the *auth* file as a whitelist (true) or a blacklist (false, value by default).

*auth* file must be without extension and put in the executable program folder. It's the file rules for newcomer acceptance or rejection.
It contains host client followed by path page (separated by a comma).

    #host, 		  path
    www.site.fr 					# match all pages of "www.site.fr"
    www.site.fr, /home/index.html 	# match only the "/home/flower.html" page of "www.site.fr"
    www.site.fr, /home/flower.swf   # match the "http://www.site.fr/home/flower.swf" usage on any web pages and host
	
If it's configured as a "blacklist", a client which matchs a line will be rejected.
If it's configured as a "whitelist", a newcomer which math a line is accepted.

The configuration file must have *CumulusService* as base name and can be a *ini*, *xml*, or *properties* file type, as you like (personnal choice of preferred style).

**CumulusService.ini**

    port = 1985 
	keepAlivePeer = 10
	keepAliveServer = 15
	[auth]
	whitelist = true

**CumulusService.xml**

    <config>
      <port>1985</port>
	  <keepAlivePeer>10</port>
	  <keepAliveServer>15</port>
	  <auth whitelist="true" />
    </config>

**CumulusService.properties**

    port = 1985
	keepAlivePeer = 10
	keepAliveServer = 15
	auth.whitelist = true

If this configuration file doesn't exist, default values will be used.

CumulusService includes some argument launch options. Command-line way is preferred during development and test usage.
The following arguments are availables:

- **registerService**,
register the application as a service.

- **unregisterService**,
unregister the application as a service.

- **displayName**,
specify a display name for the service (only with /registerService).

- **startup=automatic|manual**,
specify the startup mode for the service (only with /registerService).

- **log=level**,
log level argument beetween 0 and 8 : none, fatal, critic, error, warn, note, info, debug, trace. Default value is 6 (info), all logs until info level are displayed.

- **dump[=middle|all]**,
enables packet traces in logs. Optional arguments are 'middle' or 'all' respectively to displays just middle packet process or all packet process.
If no argument is given, just outside packet process will be dumped.

- **cirrus=address**,
cirrus address to activate a 'man-in-the-middle' developer mode in bypassing flash packets to the official cirrus server of your choice, it's a instable mode to help Cumulus developers, "p2p.rtmfp.net:10007" for example.
By adding the 'dump' argument, you will able to display Cirrus/Flash packet exchange in your logs (see 'dump' argument).

- **middle**,
Enables a 'man-in-the-middle' developer mode between two peers. It's a instable mode to help Cumulus developers.
By adding the 'dump' argument, you will able to display Flash/Flash packet exchange in your logs (see 'dump' argument).

- **help**,
displays help information about command-line usage.


### Flash side

Flash client connect to Cumulus by the classical NetConnection way:

    _netConnection.connect("rtmfp://localhost/");

Here the port has its default value 1935. If you configure a different port on CumulusService you must indicate this port in the URL (after localhost, of course).

In "man-in-the-middle" mode (see command-line argument *cirrus* in usage part) you must indacted on side flash your Cirrus key developer.
	
	_netConnection.connect("rtmfp://localhost/KEY");
	
Of course "KEY" must be replaced by your Cirrus development key.

If *NetStatusEvent.NET_STATUS* event from *NetConnection* object has failed, it may be due to:

- Bad server address
- Client has no permissions
- "netConnection.objectEncoding" has perhaps been forced to "ObjectEncoding.AMF0" value, and it doesn't support by Cumulus.

__notice:__ The *ipMulticastMemberUpdatesEnabled* NetGroup mode is not supporter for this moment.

### CumulusLib

CumulusService is almost a empty project, and all CumulusLib usage is included in main.cpp file.
Looks its file content is still the best way to learn to play with ;-)

Quickly you can handle connection/fail/disconnection client, and exchange custom data with him. Also you can accepted or rejected a newcomer client.
A brief overview:

    #include "RTMFPServer.h"

    using namespace Cumulus;
	
	class ClientHandler: private ClientHandler {
	public:
		ClientHandler(){}
		bool onConnection(Client& client) {
			// Here you can read custom client http parameters in reading "client.parameters"
			map<string,string>::const_iterator it = client.parameters.find("name");
			string name = it==client.parameters.end() ? "unknown" : it->second;
			// Also you can send custom data for the client in writing in "client.data",
			// on flash side you could read that on "data" property from NetStatusEvent::NET_STATUS event of NetConnection object
			client.data.resize(name.size()+6);
			memcpy(&client.data[0],string("Hello " + name).c_str(),client.data.size());
			...
			return connectionAccepted;
		}
		void onFailed(const Client& client,const std::string& msg) {
			...
		}
		void onDisconnection(const Client& client) {
			...
		}
	};
	
	ClientHandler clientHandler;
    RTMFPServer server(clientHandler);
    server.start();
    ...
    server.stop();
	

Build
------------------------------------

Cumulus source code is crossplatform.

### Dependencies

Cumulus has the following dependencies:

- [OpenSSL] is required.

- [Poco] in its Basic edition is required.

### Building

**Windows**

Visual Studio 2008 file solutions and projects are included.
It finds the external librairies in "External/lib" folder and external includes in "External/include" folder at the root project.
So you must put Poco and OpenSSL includes/libs in these folders.
You can find OpenSSL binaries for windows on [Win32OpenSSL].
Poco builds with Visual Studio interpreter command line (see its readme file about *buildwin.cmd*),
but this is a example for Visual Studio 2010 which build quickly (just in static mode and without Poco samples) :

	buildwin 100 build static_mt both Win32 nosamples devenv

**Linux/Unix**

If your linux system includes a package manager you can install fastly OpenSSL and Poco dependencies,
package names are *libssl-dev* and *libpoco-dev*.

CumulusLib building manipulation are:

	make            // install
	make clean      // uninstall

CumulusService works in a same way:

	make            // install
	make clean      // uninstall

Thanks
------------------------------------
Special thanks to Key2 and Andrei of [C++ RMTP Server] who by their preliminary work has made this project possible.


[C++ RMTP Server]: [http://www.rtmpd.com] "www.rtmpd.com"
[GNU General Public License]: http://www.gnu.org/licenses/ "www.gnu.org/licenses"
[license]: https://github.com/OpenRTMFP/Cumulus/raw/master/LICENSE "LICENSE"
[OpenSSL]: http://www.openssl.org/ "www.openssl.org"
[Poco]: http://pocoproject.org/ "pocoproject.org" 
[Win32OpenSSL]: [http://www.slproweb.com/products/Win32OpenSSL.html] "www.slproweb.com"
[us]: https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=M24B32EH2GV3A "Donation US"
[eu]: https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=QPWT9V67YWSGG "Donation EU"
