# source files.
OBJECTS = Address AESEngine AMFSimpleObject AMFObjectWriter AMFReader AMFWriter BinaryStream BinaryReader BinaryWriter Client Cookie Cumulus Edges EdgeSession Entity Flow FlowConnection FlowGroup FlowNull FlowStream FlowWriter Group Handshake Invoker Listener Logs MemoryStream Message Middle PacketReader PacketWriter Peer Publication Publications QualityOfService RTMFP RTMFPServer RTMFPServerEdge ServerConnection ServerSession Session Sessions Startable Streams Target Trigger Util

CC=g++
ifeq ($(shell uname -s),Darwin)
    LIB=libCumulus.dylib
	SHARED=-dynamiclib
else
	LIB=libCumulus.so
	SHARED=-shared
endif
SOURCES=./sources/
CFLAGS+=-D CUMULUS_LOGS
INCLUDES = -I./include/
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating dynamic lib $(LIB)
	@$(CC) $(SHARED) $(OBJECT) -o $(LIB)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) -fpic $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(LIB)
