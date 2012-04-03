OBJECTS = main Blacklist FileWatcher LUAAMFObjectWriter LUAByteReader LUAByteWriter LUAClient LUAClients LUAEdges LUAFlowWriter LUAGroup LUAGroups LUAInvoker LUAListener LUAListeners LUAMail LUAPublication LUAPublications LUAQualityOfService LUATCPClient LUATCPServer Script Server Service SMTPSession SocketManager TCPClient TCPServer

CC=g++
EXEC=CumulusServer
INCLUDES=-I/usr/local/include/ -I./../CumulusLib/include/
LIBDIR=-L/usr/local/lib/ -L./../CumulusLib/
SOURCES=./sources/
CFLAGS+=-D CUMULUS_LOGS
LDFLAGS+="-Wl,-rpath,./../CumulusLib/,-rpath,/usr/local/lib/"
LIBS ?= -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lcrypto -lssl -llua5.1
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating CumulusServer executable $(EXEC)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LIBDIR) -o $(EXEC) $(OBJECT) $(LIBS)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(EXEC)
