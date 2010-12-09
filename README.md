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




















