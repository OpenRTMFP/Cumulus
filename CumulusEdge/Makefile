OBJECTS = main

CC=g++
EXEC=CumulusEdge
INCLUDES=-I/usr/local/include/ -I./../CumulusLib/include/
LIBDIR=-L/usr/local/lib/ -L./../CumulusLib/
SOURCES=./sources/
CFLAGS+=-D CUMULUS_LOGS
LDFLAGS+="-Wl,-rpath,./../CumulusLib/,-rpath,/usr/local/lib/"
LIBS ?= -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lcrypto -lssl
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating CumulusEdge executable $(EXEC)
	@$(CC) $(CFLAGS) $(LDFLAGS) $(LIBDIR) -o $(EXEC) $(OBJECT) $(LIBS)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(EXEC)
