# source files.
LIBSRC = AESEngine.cpp AMFReader.cpp AMFWriter.cpp BLOB.cpp Cirrus.cpp Cookie.cpp Cumulus.cpp Database.cpp Flow.cpp FlowNetConnection.cpp FlowNetStream.cpp FlowNull.cpp Gateway.cpp Handshake.cpp Logs.cpp MemoryStream.cpp Middle.cpp PacketReader.cpp PacketWriter.cpp RTMFP.cpp RTMFPServer.cpp ServerData.cpp Session.cpp Sessions.cpp Util.cpp

CC=g++
AR=ar
LIB=libCumulus.so
LIBDEST=./
SOURCE_DIR=./sources/
CFLAGS=-g -D CUMULUS_EXPORTS
INCLUDES = -I./sources/ -I./include/  
LIBS = -L/usr/lib -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lPocoData -lPocoSQLite -lssl
LIBOBJ=$(LIBSRC:%.cpp=%.o)

$(LIBOBJ): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) -fpic $(CFLAGS) $(INCLUDES) $(LIBS) -c -o $@ $(SOURCE_DIR)$(@:%.o=%.cpp)

$(LIB): $(LIBOBJ)
	@echo creating dynamic lib $(LIB)
	@$(CC) -shared $(LIBS) $(LIBOBJ) -o $(LIB)

clean:
	rm -f $(LIBOBJ) $(LIB)

install:
	cp $(LIB) /usr/lib/$(LIB)