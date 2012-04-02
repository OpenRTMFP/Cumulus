# source files.
OBJECTS = Address AESEngine AMFSimpleObject AMFObjectWriter AMFReader AMFWriter BinaryStream BinaryReader BinaryWriter Client Cookie Cumulus Edges EdgeSession Entity Flow FlowConnection FlowGroup FlowNull FlowStream FlowWriter Group Handshake Invoker Listener Logs MemoryStream Message Middle PacketReader PacketWriter Peer Publication Publications QualityOfService RTMFP RTMFPServer RTMFPServerEdge SendingEngine SendingThread ServerConnection ServerSession Session Sessions Startable Streams Target Trigger Util

CC=g++
ifeq ($(shell uname -s),Darwin)
    LIB=libCumulus.dylib
	SHARED=-dynamiclib -install_name ./../CumulusLib/$(LIB)
else
	LIB=libCumulus.so
	SHARED=-shared
endif
SOURCES=./sources/
CFLAGS+=-D CUMULUS_LOGS
LIBS ?= -L/usr/local/lib/ -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lcrypto -lssl
INCLUDES = -I/usr/local/include/ -I./include/
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating dynamic lib $(LIB)
	@$(CC) -fPIC $(SHARED) $(OBJECT) -o $(LIB) $(LIBS)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) -fpic $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(LIB)
