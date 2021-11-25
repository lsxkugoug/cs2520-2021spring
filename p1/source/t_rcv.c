#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();
static void Clear_sock(int index);

static int Port;
static int Recv_socks;
static int Valid;  
static fd_set Read_mask;

int main(int argc, char *argv[])
{
    struct sockaddr_in name;
    int                listen_sock;
    fd_set             mask;
    int                i,num;
    int                bytes_read = 0;
    int                ret;
    char               mess_buf[MAX_MESS_LEN];
    long               on=1;
    struct timeval now;
    struct timeval last;
    gettimeofday(&last, NULL);

    int fd; 
    int isname = 1; 
    /* Parse commandline args */
    Usage(argc, argv);
    printf("Listening for connections on port %d\n", Port);

    /* Open socket to listen for connections */
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("tcp_server: socket");
        exit(1);
    }

    /* Allow binding to same local address multiple times */
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        perror("Net_server: setsockopt error \n");
        exit(1);
    }

    /* Bind to listen on given port */
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(Port);
    if (bind(listen_sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("tcp_server: bind");
        exit(1);
    }

    /* Start listening */
    if (listen(listen_sock, 4) < 0) {
        perror("tcp_server: listen");
        exit(1);
    }

    /* Init mask of file descriptors we're interested in */
    FD_ZERO(&Read_mask);
    FD_SET(listen_sock, &Read_mask);

    i = 0;
    for(;;)
    {
        /* (Re)set mask */
        mask = Read_mask;

        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0) {
	gettimeofday(&last, NULL);
            /* Accept a new connection */
            if (FD_ISSET(listen_sock, &mask)) {
                Recv_socks = accept(listen_sock, 0, 0) ;
                FD_SET(Recv_socks, &Read_mask);
                Valid = 1;
                i++;
            }

                if (Valid && FD_ISSET(Recv_socks, &mask)) {
                    
                    printf("=====\n");
                    while(1){
                        ret = recv(Recv_socks, &mess_buf, MAX_MESS_LEN, 0);
                        if(isname){
                            mess_buf[ret] = '\0';
                            printf("the file name is %s\n", mess_buf);
                            fd = open(mess_buf,O_RDWR| O_CREAT,0777);
                            isname = 0;
                            continue;
                        }
                        bytes_read += ret;
                        if (ret <= 0) {
                            gettimeofday(&now, NULL);
                            // Clear_sock(j);
                            FD_CLR(Recv_socks, &Read_mask);
                            close(Recv_socks);
                            close(fd);
                            int megb = bytes_read/	1000000;
                            int sec = now.tv_sec - last.tv_sec;
                            printf("write %d Megabytes\n",megb);
                            printf("the time use:%ds\n", sec);
                            printf("the transmit rate:%d M/s\n", megb/sec);
                            return 0;
                        }
                        
			            write(fd, mess_buf, ret);
                       
                        
                    }
                }
        }
    }

    return 1;

}


/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    if (argc != 2) {
        Print_help();
    }

    if (sscanf(argv[1], "%d", &Port) != 1) {
        Print_help();
    }
}

static void Print_help() {
    printf("Usage: tcp_server <port>\n");
    exit(0);
}
