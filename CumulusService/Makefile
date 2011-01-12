CC=g++
EXEC=CumulusService
CFLAGS=-D CUMULUS_EXPORTS
LIB=libCumulus.so
LIBS = -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lssl

main: 
	@echo compiling main.cpp
	@$(CC) $(CFLAGS) -B./../CumulusLib/ $(LIBS) -o $(EXEC) main.cpp
	cp ./../CumulusLib/$(LIB) ./$(LIB)

clean:
	rm -f $(EXEC) ./$(LIB)