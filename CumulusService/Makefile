CC=g++
EXEC=CumulusService
LIBS = -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lssl

main: 
	@echo compiling main.cpp
	@$(CC) -B./../CumulusLib/ $(LIBS) -o $(EXEC) main.cpp

clean:
	rm -f $(EXEC)