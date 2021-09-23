CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: udp_server udp_client tcp_client tcp_server file_copy simple_tcp_client simple_tcp_server udp_server_hdr udp_client_hdr

udp_server: udp_server.o
	    $(CC) -o udp_server udp_server.o  

udp_client: udp_client.o
	    $(CC) -o udp_client udp_client.o

tcp_server: tcp_server.o
	    $(CC) -o tcp_server tcp_server.o  

tcp_client: tcp_client.o
	    $(CC) -o tcp_client tcp_client.o

simple_tcp_server: simple_tcp_server.o
	    $(CC) -o simple_tcp_server simple_tcp_server.o  

simple_tcp_client: simple_tcp_client.o
	    $(CC) -o simple_tcp_client simple_tcp_client.o

file_copy: file_copy.o
	    $(CC) -o file_copy file_copy.o

udp_server_hdr: udp_server_hdr.o
	    $(CC) -o udp_server_hdr udp_server_hdr.o  

udp_client_hdr: udp_client_hdr.o
	    $(CC) -o udp_client_hdr udp_client_hdr.o

clean:
	rm *.o
	rm udp_server 
	rm udp_client
	rm tcp_server 
	rm tcp_client
	rm simple_tcp_server 
	rm simple_tcp_client
	rm file_copy
	rm udp_server_hdr
	rm udp_client_hdr

%.o:    %.c
	$(CC) $(CFLAGS) $*.c


