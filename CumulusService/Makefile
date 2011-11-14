OBJECTS = main Auth Server StatusWriter

CC=g++
EXEC=CumulusService
INCLUDES=-I./../CumulusLib/include/
SOURCES=./sources/
CFLAGS+=-D CUMULUS_EXPORTS
LIBS ?= -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lssl
OBJECT = $(OBJECTS:%=%.o)

main: $(OBJECT)
	@echo creating CumulusService executable $(EXEC)
	@$(CC) $(CFLAGS) $(LIBS) -o $(EXEC) $(OBJECT)

$(OBJECT): 
	@echo compiling $(@:%.o=%.cpp)
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $(@) $(SOURCES)$(@:%.o=%.cpp)

clean:
	rm -f $(OBJECT) $(EXEC)
