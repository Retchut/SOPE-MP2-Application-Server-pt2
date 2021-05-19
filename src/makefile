all: s c

s: server.c cmd_parser.c
	gcc -Wall -DDELAY=100 -pthread -o s server.c cmd_parser.c 

c: client.c common.h cmd_parser.c timer.c
	gcc -Wall -pthread -o c client.c cmd_parser.c timer.c 

# server.o: server.c common.h
# 	gcc -Wall -c -o server.o server.c
#
# lib.o: lib.c lib.h
# 	gcc -Wall -c -o lib.o lib.c

clean:
	rm -f s c
