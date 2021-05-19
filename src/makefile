all: s c

s: server.c cmd_parser.c delay.h delay.c lib.o lib.h
	gcc -Wall -DDELAY=100 -pthread -o s server.c cmd_parser.c 

c: client.c common.h
	gcc -Wall -o c client.c -pthread

lib.o: lib.c lib.h
	gcc -Wall -c -o lib.o lib.c

clean:
	rm -f s c lib.o

