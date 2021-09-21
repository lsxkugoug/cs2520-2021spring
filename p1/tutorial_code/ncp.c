#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static const struct timeval Zero_time = {0, 0};
static char* Source_file_name;
static char* Dest_file_name;
static int Port;
static char *Receiver_IP;
static char window[101][MAX_MESS_LEN];
static int head = 0;
static int tail = 1;

// Sender Process
int main(int argc, char **argv) { /* udp related */
    struct sockaddr_in send_addr;
    struct sockaddr_in from_addr;
    socklen_t from_len;
    struct hostent h_ent;
    struct hostent *p_h_ent;
    int host_num;
    int from_ip;
    int sock;
    fd_set mask;
    fd_set read_mask;
    int bytes;
    int num;
    char mess_buf[MAX_MESS_LEN];
    uhdr *hdr = (uhdr *) mess_buf;
    char *data_buf = &mess_buf[sizeof(uhdr)];
    int seq = 0;
    struct timeval        last_recv_time = {0, 0};
    struct timeval nowtime = {0,0};
    /* file related */
    // FILE *fr;      /* Pointer to source file */

    /* Get command line argument: <dest_file_name>@<ip_address>:<port> */
    Usage(argc, argv);

    /* Open Our Socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("ncp: socket");
        exit(1);
    }

    /* Convert string IP address (or hostname) to format we need */
    p_h_ent = gethostbyname(Receiver_IP);
    if (p_h_ent == NULL) {
        printf("udp_client: gethostbyname error.\n");
        exit(1);
    }

    memcpy(&h_ent, p_h_ent, sizeof(h_ent));
    memcpy(&host_num, h_ent.h_addr_list[0], sizeof(host_num));
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = host_num;
    send_addr.sin_port = htons(Port);

    /* Open the source file for reading */
    // if ((fr = fopen(argv[1], "r")) == NULL) {
    //     perror("fopen");
    //     exit(0);
    // }
    printf("Opened %s for reading...\n", argv[1]);

    /* Set up mask for file descriptors we want to read from */
    int fd = open(Source_file_name, O_RDONLY | O_CREAT);
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);
    FD_SET(fd, &read_mask);

    /* Send the file name to Receiver */
    // memcpy(mess_buf, Dest_file_name, strlen(Dest_file_name) + 1);

    /* 0： have not transmitted the filename to receiver
     * 1:  Already transmitted the filename to receiver
     * */
    int filename_send = 0;

    seq = 0; // seq: index of message packet
    for (;;) {

        /* Read in a chunk of the file, firstly read the name, then the file */


        /* (Re)set mask and timeout */
        mask = read_mask;

        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0) {
            // while(head!=tail || nowtime.tv_sec-last_recv_time.tv_sec > 1)
            while(1){ 
                if(nowtime.tv_sec-last_recv_time.tv_sec > 1){ //TODO send all the packages of the windows
                    for(int i = 0; i < 101; i++){
                        if(i!=head)
                            sendto(sock, window[i], sizeof(uhdr) + strlen(data_buf), 0,
                            (struct sockaddr *) &send_addr, sizeof(send_addr));
                    }
                    last_recv_time = nowtime;
                    gettimeofday(&nowtime,NULL);
                    continue;
                }
                gettimeofday(&nowtime, NULL);
                /* Receive Response from Receiver */
                if (FD_ISSET(sock, &mask)) {
                    from_len = sizeof(from_addr);
                    bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,
                                    (struct sockaddr *) &from_addr,
                                    &from_len);
                    // mess_buf[bytes] = '\0';
                    from_ip = from_addr.sin_addr.s_addr;
                    // process ack
                    uhdr *minimal_seq_pack = (uhdr *) window[head+1];
                    int mini_seq = minimal_seq_pack->seq;
                    int gap = hdr->cack - mini_seq; 
                    if(gap >= 0){ // window slide
                        head = (head + gap + 1)%101;
                        gettimeofday(&last_recv_time, NULL);
                        gettimeofday(&nowtime, NULL);
                    }
                        

                    // process nack
                    for(int i = 0; i < 5; i++){
                        int seq_need = (hdr->nack)[i];
                        int seq_gap = seq_need - mini_seq;
                        if(seq_gap>=0){
                            int seq_need_index = (head+seq_gap+1)%101;
                            sendto(sock, window[seq_need], sizeof(uhdr) + strlen(data_buf), 0,
                            (struct sockaddr *) &send_addr, sizeof(send_addr));
                        }
                    }
                }
                    /* Send the message to Receiver */
                else if (FD_ISSET(fd, &mask)) {
                    while(head!=tail){
                    /*Send the file name*/
                        if (filename_send == 0) {
                            strcpy(data_buf, Source_file_name);
                            filename_send = 1;
                            printf("%s\n",data_buf);
                            /* Send Message */
                        } else {
                            /* Put data into message*/
                            bytes = read(fd, data_buf, sizeof(mess_buf) - sizeof(uhdr));
                        }

                        /* Fill in header info */
                        hdr->seq = seq++;
                        gettimeofday(&hdr->ts, NULL);
                        hdr->cack = 1; // It does not matter in header
                        // hdr->nack = 1;

                        /* Send the message */
                        sendto(sock, mess_buf, sizeof(uhdr) + strlen(data_buf), 0,
                            (struct sockaddr *) &send_addr, sizeof(send_addr));
                        memcpy(&window[tail], mess_buf, sizeof(mess_buf)); // cash it
                        tail = (tail+1)%101; // move tail
                    }
                }
            }

        }
    }

    // ToDo : close the File when finish writing
    /* fread returns a short count either at EOF or when an error occurred */
    /* if (nread < MAX_MESS_LEN) {
        if (feof(fr)) {
            printf("Finished writing.\n");
            break;
        }else {
            printf("An error occurred...\n");
            exit(0);
        }
        }
    */


    /* Cleanup */
    // fclose(fr);
    close(fd);

    return 0;
}

/* Read commandline arguments <dest_file_name>@<ip_address>:<port>  */
static void Usage(int argc, char *argv[]) {
    if (argc != 3){
        printf("Usage: ncp <source_file_name> <dest_file_name>@<ip_address>:<port>\n");
        exit(0);
    }
    Source_file_name = argv[1];
    char* delim = "@:";
    Dest_file_name = strtok(argv[2], delim);
    Receiver_IP = strtok(NULL,delim);
    Port = atoi(strtok(NULL,delim));
    printf("Sending to %s at port %d\n", Receiver_IP, Port);
}

/* Returns 1 if t1 > t2, -1 if t1 < t2, 0 if equal */
static int Cmp_time(struct timeval t1, struct timeval t2)
{
  if (t1.tv_sec > t2.tv_sec)
    return 1;
  else if (t1.tv_sec < t2.tv_sec)
    return -1;
  else if (t1.tv_usec > t2.tv_usec)
    return 1;
  else if (t1.tv_usec < t2.tv_usec)
    return -1;
  else
    return 0;
}
