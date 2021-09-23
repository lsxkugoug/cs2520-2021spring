#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static const struct timeval Zero_time = {0, 0};
static char* Source_file_name;
static char* Dest_file_name;
static int Loss_rate;
static int Port;
static char *Receiver_IP;

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

    int seq;
    struct timeval last_recv_time = {0, 0};
    struct timeval nowtime = {0, 0};
    struct timeval timeout;
    char window[WINDOW_SIZE][MAX_MESS_LEN];
    int head = 1;
    int tail = WINDOW_SIZE;

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

    printf("Opened %s for reading...\n", argv[1]);

    /* Set up mask for file descriptors we want to read from */
    int fd = open(Source_file_name, O_RDONLY | O_CREAT);
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);
    FD_SET(fd,&read_mask);

    /* 0： have not transmitted the filename to receiver
     * 1:  Already transmitted the filename to receiver
     * */
    int filename_send = 0;

    seq = 0; // seq: index of message packet

    int eof = 0; /* 0:Does not reach the end of file 1: Read the end of file */

    /* Initialization : First send the file name
     * Secondly, Read data from file and fill in the Window ,then send the whole window */
    strcpy(data_buf, Source_file_name);
    hdr->seq = seq++;
    sendto_dbg(sock, mess_buf, sizeof(uhdr) + strlen(data_buf), 0,
           (struct sockaddr *) &send_addr, sizeof(send_addr));
    for (int i = head; i <= tail; i++) {
        bytes = read(fd, data_buf, sizeof(mess_buf) - sizeof(uhdr));
        hdr->seq = seq++;
        // Send Message
        sendto_dbg(sock, mess_buf, sizeof(uhdr) + strlen(data_buf), 0,
               (struct sockaddr *) &send_addr, sizeof(send_addr));
        // Store Message in window
        memcpy(window[i % WINDOW_SIZE], mess_buf, sizeof(mess_buf));
    }

    for (;;) {

        /* (Re)set mask and timeout */
        mask = read_mask;

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
        printf("Num,%d , EOF %d \n",num,eof);

        /* No Response From Receiver */

        /* Receive Response from Receiver  (num = 1  eof=1)=> receiver  num==0 No receiver, No file  */
        /* (eof==1&&num==1)  num= 1 eof = 0*/
        if(num==2){
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,
                                 (struct sockaddr *) &from_addr,
                                 &from_len);
                from_ip = from_addr.sin_addr.s_addr;
                printf("Receive ACK from receiver:%d, head:%d,tail:%d\n", hdr->cack, head, tail);

                /* Check whether we finish sending the file*/
                if (eof == 1 && (hdr->cack >= tail || head >= tail)) {
                    printf("Finish Sending the file ....");
                    close(fd);
                    exit(0);
                }

                int received_ack = hdr->cack;

                /* When Cumulative ACK is below than head,We may lose some packet. Check Nack array,and Resend Message */
                if (received_ack < head) {
                    for (int i = 0; i < NACK_SIZE; i++) {
                        if (hdr->nack[i] < 0) {
                            break;
                        } else {
                            if (head <= hdr->nack[i] && hdr->nack[i] <= tail) {
                                sendto_dbg(sock, window[hdr->nack[i] % WINDOW_SIZE],
                                       sizeof(window[hdr->nack[i] % WINDOW_SIZE]), 0,
                                       (struct sockaddr *) &send_addr, sizeof(send_addr));
                            }
                        }
                    }
                }
                    /* Window [head, head+1.....tail-1,tail] */
                    /* When Cumulative ACK is above head-1, shift window,send new message */
                else if (received_ack >= head) {
                    if (eof == 1) {
                        /* Nothing to read, Just move the head */
                        head = received_ack + 1;
                    } else {
                        /* Otherwise, shit window, increase head to Received ack+1
                         * Meanwhile, we increase tail and read from file into buffer and send them to Receiver
                         * */
                        for (int i = head;  i < received_ack + 1; i++) {

                            bytes = read(fd, data_buf, sizeof(mess_buf) - sizeof(uhdr));

                            // Check whether Read the end of the file
                            if (bytes < sizeof(mess_buf) - sizeof(uhdr)) {
                                printf("Finished Reading .\n");
                                eof = 1;
                               // close(fd);
                            }
                            hdr->seq = seq++;

                            // Store Message in window sizeof(mess_buf) vs sizeof(uhdr)+strlen(data_buf)
                            memcpy(window[(tail + 1) % WINDOW_SIZE], mess_buf, sizeof(mess_buf));

                            // Send Message
                            sendto_dbg(sock, mess_buf, sizeof(uhdr) + strlen(data_buf), 0,
                                   (struct sockaddr *) &send_addr, sizeof(send_addr));

                            head++;
                            tail++;

                            // Reset the Timer
                            timeout.tv_sec = 1;
                            timeout.tv_usec = 0;

                            if(eof ==1 ){
                                head = received_ack+1;
                                break;
                            }
                        }
                    }
                }
            }
        }else{
            if (head == tail) {
                close(fd);
                exit(0);
            }
            /* Time out, Resend everything in buffer to Receiver */
            for (int i = head; i <= tail; i++) {
                // Send Message
                sendto_dbg(sock, window[i % WINDOW_SIZE], sizeof(window[i % WINDOW_SIZE]), 0,
                       (struct sockaddr *) &send_addr, sizeof(send_addr));
            }
        }

    }
    return 0;
}

/* Read commandline arguments <dest_file_name>@<ip_address>:<port>  */
static void Usage(int argc, char *argv[]) {
    if (argc != 4){
        printf("Usage: ncp <loss_rate> <source_file_name> <dest_file_name>@<ip_address>:<port>\n");
        exit(0);
    }
    Loss_rate = atoi(argv[1]);
    sendto_dbg_init(Loss_rate);
    Source_file_name = argv[2];
    char* delim = "@:";
    Dest_file_name = strtok(argv[3], delim);
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
