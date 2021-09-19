#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();

static char *Server_IP;
static int Port;

int main(int argc, char *argv[])
{
    struct sockaddr_in host;
    struct hostent     h_ent, *p_h_ent;

    int                sock;
    int                ret;
    int                bytes_sent;
    int                mess_len;
    char               mess_buf[MAX_MESS_LEN];
    char               *data_mess_ptr = &mess_buf[sizeof(mess_len)]; 

    /* Parse commandline args */
    Usage(argc, argv);
    printf("Sending to %s at port %d\n", Server_IP, Port);

    /* Open socket 
    * ipv4 AF_INET
    */

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("tcp_client: socket error\n");
        exit(1);
    }

    /* Set up server address to send to */
    p_h_ent = gethostbyname(Server_IP);
    if (p_h_ent == NULL) {
        printf("tcp_client: gethostbyname error.\n");
        exit(1);
    }
    memcpy(&h_ent, p_h_ent, sizeof(h_ent));
    memcpy(&host.sin_addr, h_ent.h_addr_list[0],  sizeof(host.sin_addr));
    host.sin_family = AF_INET;
    host.sin_port   = htons(Port); 

    /* Connect to server */
    ret = connect(sock, (struct sockaddr *)&host, sizeof(host));
    if (ret < 0)
    {
        perror("tcp_client: could not connect to server\n"); 
        exit(1);
    }

    for(;;)
    {
        /* Read message into mess_buf, leaving space for message length the
         * beginning of the buffer */
        printf("enter message: ");
        fgets(data_mess_ptr, sizeof(mess_buf)-sizeof(mess_len), stdin);
        mess_len = strlen(data_mess_ptr) + sizeof(mess_len);
        /* Put message length into beginning of message buffer */
        memcpy(mess_buf, &mess_len, sizeof(mess_len));

        /* Send message */
        bytes_sent = 0;
        while (bytes_sent < mess_len) {
            ret = send(sock, &mess_buf[bytes_sent], mess_len-bytes_sent, 0);
            if (ret < 0) {
                perror("tcp_client: error in sending\n");
                exit(1);
            }
            bytes_sent += ret;
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
