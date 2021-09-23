CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: ncp rcv t_ncp t_rcv sendto_dbg

sendto_dbg: sendto_dbg.o
	    $(CC) -o sendto_dgb sendto_dgb.o	

ncp: ncp.o
	    $(CC) -o ncp sendto_dgb ncp.o

rcv: rcv.o
	    $(CC) -o rcv rcv.o

t_ncp: t_ncp.o
	    $(CC) -o t_ncp t_ncp.o

t_rcv: t_rcv.o
	    $(CC) -o t_rcv t_rcv.o




clean:
	rm ncp
	rm rcv
	rm t_ncp
	rm t_rcv
	rm sendto_dbg

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


