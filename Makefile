LIB_SOURCE  = ./src/libs/
MAIN_SOURCE = ./src/main/
CC = gcc

CFLAGS  = -c -I../inc/
LDFLAGS = -lssl -lcrypto 
PROGRAM = minicurl

all: _bin
	cd build; \
	mv $(PROGRAM) ../ 
_bin: _libs
	cd $(MAIN_SOURCE); \
	$(CC) $(CFLAGS) minicurl.c; \
	mv *.o ../../build ; \
	cd ../../build ; \
	$(CC) -o $(PROGRAM) *.o $(LDFLAGS);
_libs:
	mkdir build ; \
	cd $(LIB_SOURCE); \
	$(CC) $(CFLAGS) *.c $(LDFLAGS); \
	mv *.o ../../build ;
install:
	rm -rf build; \
	mv $(PROGRAM) /usr/sbin;
clean:
	rm -rf build; \
	rm $(PROGRAM); \
