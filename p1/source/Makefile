CC=gcc



CFLAGS = -g -c -Wall -pedantic



all: ncp rcv t_ncp t_rcv 



ncp: ncp.o sendto_dbg.c 

	    $(CC) ncp.o sendto_dbg.c  -o ncp 

 

rcv: rcv.o sendto_dbg.c 

	    $(CC) rcv.o sendto_dbg.c -o rcv 



t_ncp: t_ncp.o

	    $(CC) -o t_ncp t_ncp.o



t_rcv: t_rcv.o

	    $(CC) -o t_rcv t_rcv.o









clean:

	rm ncp

	rm rcv

	rm t_ncp

	rm t_rcv

	rm *.o




%.o:    %.c

	$(CC) $(CFLAGS) $*.c





