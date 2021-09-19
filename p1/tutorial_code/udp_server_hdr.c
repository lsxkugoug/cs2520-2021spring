#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);

static const struct timeval Zero_time = {0, 0};

static int Port;

int main(int argc, char *argv[])
{
    struct sockaddr_in    name;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    int                   from_ip;
    int                   sock;
    fd_set                mask;
    fd_set                read_mask;
    int                   bytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    struct timeval        timeout;
    struct timeval        last_recv_time = {0, 0};
    struct timeval        now;
    struct timeval        diff_time;
    uhdr                  *hdr = (uhdr *)mess_buf;
    char                  *data_buf = &mess_buf[sizeof(uhdr)];

    /* Parse commandline args */
    Usage(argc, argv);
    printf("Listening for messages on port %d\n", Port);

    /* Open socket for receiving */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("udp_server: socket");
        exit(1);
    }

    /* Bind receive socket to listen for incoming messages on specified port */
    name.sin_family = AF_INET; 
    name.sin_addr.s_addr = INADDR_ANY; 
    name.sin_port = htons(Port);

    if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("udp_server: bind");
        exit(1);
    }

    /* Set up mask for file descriptors we want to read from */
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    for(;;)
    {
        /* (Re)set mask and timeout */
        mask = read_mask;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;

        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if (num > 0) {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len);
                mess_buf[bytes] = '\0'; /* ensure string termination for nice printing to screen */
                from_ip = from_addr.sin_addr.s_addr;

                /* Record time we received this msg */
                gettimeofday(&last_recv_time, NULL);

                printf("Received from (%d.%d.%d.%d): %s\n", 
                           (htonl(from_ip) & 0xff000000)>>24,
                           (htonl(from_ip) & 0x00ff0000)>>16,
                           (htonl(from_ip) & 0x0000ff00)>>8,
                           (htonl(from_ip) & 0x000000ff),
                           data_buf);
                printf("Seq: %d, Timestamp %ld %d\n\n", hdr->seq, hdr->ts.tv_sec, hdr->ts.tv_usec);

                /* Echo message back to sender */
                sendto(sock, mess_buf, bytes, 0, (struct sockaddr *)&from_addr,
                       sizeof(from_addr));

            }
        } else {
            printf("timeout...nothing received for 10 seconds.\n");
            gettimeofday(&now, NULL);
            if (Cmp_time(last_recv_time, Zero_time) > 0) {
                timersub(&now, &last_recv_time, &diff_time);
                printf("last msg received %lf seconds ago.\n\n",
                        diff_time.tv_sec + (diff_time.tv_usec / 1000000.0));
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
    printf("Usage: udp_server <port>\n");
    exit(0);
}

/* Returns 1 if t1 > t2, -1 if t1 < t2, 0 if equal */
static int Cmp_time(struct timeval t1, struct timeval t2) {
    if      (t1.tv_sec  > t2.tv_sec) return 1;
    else if (t1.tv_sec  < t2.tv_sec) return -1;
    else if (t1.tv_usec > t2.tv_usec) return 1;
    else if (t1.tv_usec < t2.tv_usec) return -1;
    else return 0;
}
