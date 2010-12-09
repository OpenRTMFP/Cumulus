Cumulus
========

Cumulus is a crossplatform "rendezvous" service to assist P2P connection in RTMFP peer's communication. In RTMFP protocol, Cumulus plays the server role.
Cumulus can be used as a library to include in your "publisher" software, or be installed as a service on your server computer.

Usage
-----

### CumulusService

CumulusService executable includes the following argument launch options, to a developement usage:

- **help**,
displays help information about command-line usage.

- **cirrus=url**,
cirrus url to activate a "man-in-the-middle" mode in bypassing flash packets to the official cirrus server of your choice.

- **dump[=file]**,
enables packet traces in the console. Optionnal 'file' argument also allows a file dumping. Often used with 'cirrus=url' option to observe flash/cirrus exchange.

- **log=level**,
log level argument beetween 0 and 8 : none, fatal, critic, error, warn, note, info, debug, trace. Default value is 6 (note), all logs until info level are displayed.

Command-line way is preferred during development and test usage. To statically configure the service the better practice is a optionnal configuration file to the installation directory. The possible configurations are:

- **port**,
equals 1935 by default (RTMFP server default port), it's the port used by CumulusService to listen incoming RTMFP requests.

- **database.connector**,
equals 'SQLite' by default, it selects the database type used. For this moment, SQLite is the only choice possible.

- **database.connectionString**,
equals 'data.db' by default, it's the connection string associated with the database used, with a SQLite database it is the file storage path.

The configuration file must have "CumulusService" as base name and can be a "ini", "xml", or "properties" file type, as you like (personnal choice of preferred style).

**CumulusService.ini**

    ;Cumulus server port
    port = 1985 
    ;Database configurations
    [database]
    connector = SQLite
    connectionString = data.db

**CumulusService.xml**

    <config>
      <port>1985</port>
      <database>
        <connector>SQLite</connector>
        <connectionString>data.db</connectionString>
      </database>
    </config>

**CumulusService.properties**

    # Cumulus server port
    port=1985
    # Database configurations
    database.connector = SQLite
    database.connectionString = data.db

If this configuration file doesn't exist, default values will be used.

### CumulusLib

CumulusService is almost a empty project, it includes just a main.cpp file which uses all CumulusLib API. Looks its file content is still the best way to learn to play with ;-)

A brief overview:

    #include "RTMFPServer.h"

    using namespace Cumulus;
    ...
    RTMFPServer server;
    server.start();
    ...
    server.stop();


Build
-----

Cumulus source code is crossplatform.

### Dependencies

Cumulus has the following dependencies:

- [OpenSSL] is required.

- [Poco] in its Complete edition but just with 'Foundation','XML','Util','Net' (Basic edition), 'Data' and 'Data/SQLite' (parts of Complete edition) components. So contrary to what is said on their website, there is no other dependencies (doesn't require OpenSSL, MySQL and ODBC).
To build [Poco] Complete edition just with these six compoments you must edit the components file on Windows to have this

        Foundation
        XML
        Util
        Net
        Data
        Data/SQLite

    On Linux/Unix you can uses "omit" argument with configure command-line.

        ./configure --omit=CppUnit,NetSSL_OpenSSL,Crypto,Data/MySQL,Data/ODBC,PageCompiler,Zip

### Building
**Windows**

Visual Studio 2008 file solutions and projects are included.

**Linux/Unix**

Cumulus has not makefile for this time, Cumulus code is thought to be crossplaform, and it should be easy to create the necessary makefiles.


[OpenSSL]:http://www.openssl.org/ "www.openssl.org"
[Poco]:http://pocoproject.org/ "pocoproject.org" 
        


















