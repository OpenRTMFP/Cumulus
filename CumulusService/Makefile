CC=g++
EXEC=CumulusService
LIB=libCumulus.so
LIBS = -lCumulus -lPocoFoundation -lPocoXML -lPocoUtil -lPocoNet -lssl

main: 
	@echo compiling main.cpp
	@$(CC) -B./../CumulusLib/ $(LIBS) -o $(EXEC) main.cpp
	cp ./../CumulusLib/$(LIB) ./$(LIB)

clean:
	rm -f $(EXEC)