OBJECTS = main Blacklist FileWatcher LUAAMFObjectWriter LUAByteReader LUAByteWriter LUAClient LUAClients LUAEdges LUAFlowWriter LUAGroup LUAGroups LUAInvoker LUAListener LUAListeners LUAPublication LUAPublications LUAQualityOfService LUATCPClient Script Server Service SocketManager TCPClient

CC=g++
EXEC=CumulusServer
INCLUDES=-I./../CumulusLib/include/
LIBDIR=-L./../CumulusLib/
SOURCES=./sources/
CFLAGS+=-D CUMULUS_LOGS
LDFLAGS+="-Wl,-rpath,./../CumulusLib/"
LIBS ?= -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lcrypto -lssl -llua5.1
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating CumulusService executable $(EXEC)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LIBDIR) -o $(EXEC) $(OBJECT) $(LIBS)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(EXEC)
