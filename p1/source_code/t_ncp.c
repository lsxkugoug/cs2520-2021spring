#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();

static char *Receiver_IP;
static int Port;

static char* Source_file_name;
static char* Dest_file_name;


int main(int argc, char *argv[])
{
    struct sockaddr_in host;
    struct hostent     h_ent, *p_h_ent;

    int                sock;
    int                ret;
    int                bytes_sent;
    int                mess_len;
    char               mess_buf[MAX_MESS_LEN];

    /* Parse commandline args */
    Usage(argc, argv);

    /* Open socket 
    * ipv4 AF_INET
    */

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("tcp_client: socket error\n");
        exit(1);
    }

    /* Set up server address to send to */
    p_h_ent = gethostbyname(Receiver_IP);
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

    int fd = open(Source_file_name, O_RDONLY | O_CREAT);

    /*send the filename*/
      
    ret = send(sock, Dest_file_name, sizeof(Dest_file_name), 0);
    printf("%s\n",Dest_file_name);

    for(;;)
    {
        int all_byte = 0;
        int bytes_sent = read(fd, mess_buf, sizeof(mess_buf));
        ret = send(sock, &mess_buf, bytes_sent, 0);
        if (ret < 0) {
            perror("tcp_client: error in sending\n");
            exit(1);
        }
        all_byte += ret;
        if(bytes_sent == 0) break;
    }

    return 0;

}

/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    if (argc != 3){
        
        
        
        
        
        
        ("Usage: t_ncp <source_file_name> <dest_file_name>@<ip_address>:<port>\n");
        exit(0);
    }
    Source_file_name = argv[1];
    char* delim = "@:";
    Dest_file_name = strtok(argv[2], delim);
    Receiver_IP = strtok(NULL,delim);
    Port = atoi(strtok(NULL,delim));
    printf("Sending to %s at port %d\n", Receiver_IP, Port);
}

static void Print_help() {
    printf("Usage: t_ncp <source_file_name> <dest_file_name>@<ip_address>:<port>\n");
    exit(0);
}
