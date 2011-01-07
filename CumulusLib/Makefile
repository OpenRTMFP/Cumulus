# source files.
OBJECTS = AESEngine AMFReader AMFWriter Cirrus Cookie Cumulus Flow FlowConnection FlowGroup FlowNull Gateway Group Handshake Logs MemoryStream Middle PacketReader PacketWriter Peer RTMFP RTMFPServer ServerData Session Sessions Util

CC=g++
LIB=libCumulus.so
SOURCES=./sources/
CFLAGS=-g -D CUMULUS_EXPORTS
INCLUDES = -I./include/
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating dynamic lib $(LIB)
	@$(CC) -shared $(OBJECTS:%=%.o) -o $(LIB)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) -fpic $(CFLAGS) $(INCLUDES) -c -o $@ $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECTS:%=%.o) $(LIB)