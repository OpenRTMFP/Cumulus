OBJECTS = main FileWatcher LUABroadcaster LUAByteReader LUAByteWriter LUAClient LUAClients LUAFlowWriter LUAGroup LUAGroups LUAInvoker LUAListener LUAListeners LUAMail LUAMember LUAPublication LUAPublications LUAQualityOfService LUAServer LUAServers LUATCPClient LUATCPServer Script Server ServerConnection ServerMessage Servers Service SMTPSession TCPClient TCPServer

CC=g++
EXEC=CumulusServer
INCLUDES=-I/usr/local/include/ -I./../CumulusLib/include/
LIBDIR=-L/usr/local/lib/ -L./../CumulusLib/
SOURCES=./sources/
CFLAGS+=-D CUMULUS_LOGS
LDFLAGS+="-Wl,-rpath,./../CumulusLib/,-rpath,/usr/local/lib/"
LIBS ?= -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lcrypto -lssl -lluajit-5.1
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating CumulusServer executable $(EXEC)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LIBDIR) -o $(EXEC) $(OBJECT) $(LIBS)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(EXEC)
