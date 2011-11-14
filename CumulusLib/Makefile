# source files.
OBJECTS = Address AESEngine AMFObject AMFObjectWriter AMFReader AMFWriter BinaryStream BinaryReader BinaryWriter Clients Cookie Cumulus Edges EdgeSession Entity Flow FlowConnection FlowGroup FlowNull FlowStream FlowWriter Group Handler Handshake Listener Logs MemoryStream Message Middle PacketReader PacketWriter Peer Peers Publication Publications QualityOfService RTMFP RTMFPServer RTMFPServerEdge ServerConnection ServerSession Session Sessions Startable Streams Target Trigger Util

CC=g++
LIB=libCumulus.so
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	LIBS = /usr/lib64/
else
	LIBS = /usr/lib/
endif
SOURCES=./sources/
CFLAGS+=-D CUMULUS_EXPORTS
INCLUDES = -I./include/
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating dynamic lib $(LIBS)$(LIB)
	@$(CC) -shared $(OBJECT) -o $(LIBS)$(LIB)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) -fpic $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(LIBS)$(LIB)
