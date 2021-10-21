CC=gcc

CFLAGS = -g -c -Wall -pedantic 

all: udp_stream udp_stream_rcv rt_rcv rt_srv

udp_stream: udp_stream.o
	    $(CC) -o udp_stream udp_stream.o

udp_stream_rcv: udp_stream_rcv.o
	    $(CC) -o udp_stream_rcv udp_stream_rcv.o

rt_rcv: rt_rcv.o sendto_dbg.c
		$(CC) -o rt_rcv rt_rcv.o sendto_dbg.c

rt_srv: rt_srv.o sendto_dbg.c
		$(CC) -o rt_srv rt_srv.o sendto_dbg.c

clean:
	rm *.o
	rm udp_stream
	rm udp_stream_rcv
	rm rt_rcv
	rm rt_srv

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


