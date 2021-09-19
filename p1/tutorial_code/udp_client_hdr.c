#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();

static char *Server_IP;
static int Port;

int main(int argc, char *argv[])
{
    struct sockaddr_in    send_addr;
    struct sockaddr_in    from_addr;
    socklen_t             from_len;
    struct hostent        h_ent;
    struct hostent        *p_h_ent;
    int                   host_num;
    int                   from_ip;
    int                   sock;
    fd_set                mask;
    fd_set                read_mask;
    int                   bytes;
    int                   num;
    char                  mess_buf[MAX_MESS_LEN];
    int                   seq;
    uhdr                  *hdr = (uhdr *)mess_buf;
    char                  *data_buf = &mess_buf[sizeof(uhdr)];

    /* Parse commandline args */
    Usage(argc, argv);
    printf("Sending to %s at port %d\n", Server_IP, Port);

    /* Open socket for sending */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("udp_client: socket");
        exit(1);
    }
    
    /* Convert string IP address (or hostname) to format we need */
    p_h_ent = gethostbyname(Server_IP);
    if (p_h_ent == NULL) {
        printf("udp_client: gethostbyname error.\n");
        exit(1);
    }
    memcpy(&h_ent, p_h_ent, sizeof(h_ent));
    memcpy(&host_num, h_ent.h_addr_list[0], sizeof(host_num));

    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num; 
    send_addr.sin_port = htons(Port);

    /* Set up mask for file descriptors we want to read from */
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);
    FD_SET((long)0, &read_mask); /* stdin */

    seq = 0;
    for(;;)
    {
        /* (Re)set mask */
        mask = read_mask;

        /* Wait for message (NULL timeout = wait forever) */
        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0) {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,  
                          (struct sockaddr *)&from_addr, 
                          &from_len);
                mess_buf[bytes] = '\0'; /* ensure string termination for nice printing to screen */
                from_ip = from_addr.sin_addr.s_addr;

                printf("Received from (%d.%d.%d.%d): %s\n", 
                           (htonl(from_ip) & 0xff000000)>>24,
                           (htonl(from_ip) & 0x00ff0000)>>16,
                           (htonl(from_ip) & 0x0000ff00)>>8,
                           (htonl(from_ip) & 0x000000ff),
                           data_buf);

            } else if (FD_ISSET(0, &mask)) {
                /* Read into data_buf (leaves space for header at the beginning
                 * of mess_buf) */
                bytes = read(0, data_buf, sizeof(mess_buf)-sizeof(uhdr));
                data_buf[bytes] = '\0';
                printf( "Read input from stdin: %s\n", data_buf );
                /* Fill in header info */
                hdr->seq = seq++;
                gettimeofday(&hdr->ts, NULL);
                /* Send the message */
                sendto(sock, mess_buf, sizeof(uhdr)+strlen(data_buf), 0, 
                       (struct sockaddr *)&send_addr, sizeof(send_addr));
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

    Server_IP = strtok(argv[1], ":");
    if (Server_IP == NULL) {
        printf("Error: no server IP provided\n");
        Print_help();
    }   
    Port = atoi(strtok(NULL, ":"));
}

static void Print_help() {
    printf("Usage: udp_client <server_ip>:<port>\n");
    exit(0);
}
