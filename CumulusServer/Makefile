OBJECTS = main FileWatcher LUABroadcaster LUAByteReader LUAByteWriter LUAClient LUAClients LUAFlowWriter LUAGroup LUAGroups LUAInvoker LUAListener LUAListeners LUAMail LUAMember LUAPublication LUAPublications LUAQualityOfService LUAServer LUAServers LUATCPClient LUATCPServer LUAUDPSocket Script Server ServerConnection ServerMessage Servers Service SMTPSession TCPClient TCPServer UDPSocket

CC=g++
EXEC=CumulusServer
INCLUDES=-I/usr/local/include/ -I./../CumulusLib/include/
LIBDIR=-L/usr/local/lib/ -L./../CumulusLib/
SOURCES=./sources/
CFLAGS+=-D CUMULUS_LOGS
LDFLAGS+="-Wl,-rpath,./../CumulusLib/,-rpath,/usr/local/lib/"
LIBS ?= -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lcrypto -lssl -lluajit-5.1

OS := $(shell uname)
ifeq ($(OS),Darwin)
	LBITS := $(shell getconf LONG_BIT)
	ifeq ($(LBITS),64)
	   # just require for OSX 64 buts
	LIBS +=  -pagezero_size 10000 -image_base 100000000
	endif
endif

OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating CumulusServer executable $(EXEC)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LIBDIR) -o $(EXEC) $(OBJECT) $(LIBS)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(EXEC)
