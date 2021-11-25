CC=gcc

CFLAGS = -c -Wall -pedantic -g

all: udp_stream udp_stream_rcv

udp_stream: udp_stream.o
	    $(CC) -o udp_stream udp_stream.o

udp_stream_rcv: udp_stream_rcv.o
	    $(CC) -o udp_stream_rcv udp_stream_rcv.o

clean:
	rm *.o

veryclean:
	rm udp_stream
	rm udp_stream_rcv

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


