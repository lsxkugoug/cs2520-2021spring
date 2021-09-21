#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();

static int Port;

int main(int argc, char *argv[])
{
    struct sockaddr_in name;
    int                listen_sock;
    int                recv_sock;
    fd_set             mask, read_mask;
    int                num;
    int                ret;
    char               mess_buf[MAX_MESS_LEN];
    long               on=1;

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

    /* Accept a connection */
    recv_sock = accept(listen_sock, 0, 0);

    /* Init mask of file descriptors we're interested in */
    FD_ZERO(&read_mask);
    FD_SET(recv_sock, &read_mask);

    for(;;)
    {
        /* (Re)set mask */
        mask = read_mask;

        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0) {
            if (FD_ISSET(recv_sock, &mask)) {
                ret = recv(recv_sock, mess_buf, sizeof(mess_buf), 0);
                if (ret <= 0) {
                    printf("closing sock\n");
                    close(recv_sock);
                    exit(0);
                }
                mess_buf[ret] = '\0';
                printf("len is : %d  message is : %s \n ", ret, mess_buf); 
                printf("---------------- \n");
            }
        }
    }

    return 0;

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
