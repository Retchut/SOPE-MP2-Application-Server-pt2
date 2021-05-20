all: s c

s: delay.h delay.c server.c cmd_parser.c lib.o lib.h queue.c timer.c
	gcc -Wall -DDELAY=100 -pthread -o s server.c cmd_parser.c queue.c timer.c lib.o delay.c

c: client.c common.h
	gcc -Wall -o c client.c -pthread

lib.o: lib.c lib.h
	gcc -Wall -c -o lib.o lib.c

clean:
	rm -f s c lib.o

