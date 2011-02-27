# source files.
OBJECTS = Address AESEngine AMFObject AMFReader AMFResponse AMFWriter Cirrus Client ClientHandler Cookie Cumulus Flow FlowConnection FlowGroup FlowNull Group Handshake Logs MemoryStream Middle PacketReader PacketWriter Peer Peers RTMFP RTMFPServer ServerData Session Sessions Util

CC=g++
LIB=libCumulus.so
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	LIBS = /usr/lib64/
else
	LIBS = /usr/lib/
endif
SOURCES=./sources/
CFLAGS=-D CUMULUS_EXPORTS
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
