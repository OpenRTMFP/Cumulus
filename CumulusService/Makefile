CC=g++
EXEC=CumulusService
CFLAGS=-D CUMULUS_EXPORTS
LIBS = -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lssl

main: 
	@echo compiling main.cpp
	@$(CC) $(CFLAGS) -B./../CumulusLib/ $(LIBS) -o $(EXEC) main.cpp

clean:
	rm -f $(EXEC)