# source files.
OBJECTS = Address AESEngine AMFObject AMFReader AMFWriter Cirrus Client Cookie Cumulus Flow FlowConnection FlowGroup FlowNull Group Handshake Logs MemoryStream Middle PacketReader PacketWriter Peer Peers RTMFP RTMFPServer ServerData Session Sessions Util

CC=g++
LIB=libCumulus.so
SOURCES=./sources/
CFLAGS=-D CUMULUS_EXPORTS
INCLUDES = -I./include/
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating dynamic lib $(LIB)
	@$(CC) -shared $(OBJECT) -o /usr/lib/$(LIB)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) -fpic $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) /usr/lib/$(LIB)