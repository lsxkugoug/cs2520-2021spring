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
    int totalbyte = 0;
    int allbytes = 0;
    int i_TMB = 0;
    int ii_TMB = 0;
    char mess_buf[MAX_MESS_LEN];
    uhdr *hdr = (uhdr *) mess_buf;
    char *data_buf = &mess_buf[sizeof(uhdr)];

    int seq = 0; // seq: index of message packet
    struct timeval last_recv_time = {0, 0};
    struct timeval nowtime = {0, 0};
    struct timeval timeout;
    struct timeval TMBtime ;
    struct  timeval now;
    struct timeval initialtime ;
    struct  timeval finishtime ;
    struct timeval diff_time;
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
    gettimeofday(&initialtime, NULL);
    gettimeofday(&TMBtime, NULL);
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);
    FD_SET(fd,&read_mask);

    int eof = 0; /* 0:Does not reach the end of file 1: Read the end of file */

    /* Connection part, sender will send filename every one second,
     * If sender receive ack = 0 then filename have been successfully sent
     * otherwise, receiver is busying with other sender. Sender has to wait until sender is free
     * */
    /* 0：have not transmitted the filename to receiver. 1:Already transmitted the filename to receiver */
    int filename_send = 0;
    strcpy(data_buf, Dest_file_name);
    gettimeofday(&initialtime, NULL);
    while(!filename_send) {
        mask = read_mask;
        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if(num>0){
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,
                                 (struct sockaddr *) &from_addr,
                                 &from_len);
                if(hdr->cack == -1){
                    puts("Receiver is handing other sender !\n");
                }
                if(hdr->cack == 0){
                    filename_send = 1;
                }
                printf("CACK :%d\n",hdr->cack);
            }
        }
        gettimeofday(&finishtime, NULL);
        timersub(&finishtime, &initialtime, &diff_time);
        if(diff_time.tv_usec >= 1){
            hdr->seq = seq;
            sendto(sock, mess_buf, sizeof(mess_buf), 0,
                   (struct sockaddr *) &send_addr, sizeof(send_addr));
            gettimeofday(&initialtime, NULL);
        }
    }
    gettimeofday(&initialtime, NULL);
    seq++;

    /* Initialization  Read data from file and fill in the Window ,then send the whole window */
    for (int i = head; i <= tail; i++) {
        bytes = read(fd, data_buf, sizeof(mess_buf) - sizeof(uhdr)-1);
        data_buf[bytes] = '\0';

        totalbyte+= bytes;
        if(totalbyte > TMB){
            i_TMB++;
            totalbyte =  totalbyte -TMB;
            gettimeofday(&now, NULL);
            timersub(&now, &TMBtime, &diff_time);
            gettimeofday(&TMBtime, NULL);
            printf("Total data I send so far : %lf MB\n",(totalbyte/1000000.0)+i_TMB*10);
            printf("Average Sending Rate: %lf MB per s\n", 10 / (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
        }
        allbytes +=bytes;
        if(allbytes>TMB){
            allbytes = allbytes -TMB;
            ii_TMB++;
        }

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

        timeout.tv_sec = NCP_T_SEC;
        timeout.tv_usec = NCP_T_USEC;

        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);

        /* Receive Response from Receiver*/
        if(num==2){
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,
                                 (struct sockaddr *) &from_addr,
                                 &from_len);
                from_ip = from_addr.sin_addr.s_addr;

                printf("Head %d, Tail %d \n",head,tail); // Todo
                printf("Response ACK %d \n",hdr->cack); //Todo

                /* Check whether we finish sending the file*/
                if (eof == 1 && (hdr->cack >= tail || head >= tail)) {
                    printf("Finish Sending the file ....\n");
                    hdr->seq = -1;
                    sendto(sock, mess_buf, sizeof(uhdr) + strlen(data_buf), 0,
                               (struct sockaddr *) &send_addr, sizeof(send_addr));
                    gettimeofday(&finishtime, NULL);
                    timersub(&finishtime, &initialtime, &diff_time);
                    printf("Sending time  %lf s.\n",
                           diff_time.tv_sec + (diff_time.tv_usec / 1000000.0));
                    printf("Total data I send : %lf MB\n",(totalbyte/1000000.0)+i_TMB*10);
                    printf("Total Bytes including junk bytes I send : %lf MB\n",allbytes/1000000.0+ii_TMB*10);
                    printf("Sending Rate: %lf MB per s\n", ((totalbyte/1000000.0)+i_TMB*10)/ (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
                    close(fd);
                    exit(0);
                }

                int received_ack = hdr->cack;

                if(received_ack <head -1){
                    /* Ignore, since we already received ACK =  head -1 */
                }
                /* When Cumulative ACK is equal to head -1,We may lose some packet. Check Nack array,and Resend Message */
                else if (received_ack == head -1) {
                    for (int i = 0; i < NACK_SIZE; i++) {
                        if (hdr->nack[i] < 0) {
                            break;
                        } else {
                            printf("Response Nack %d \n",hdr->nack[i]);// Todo
                            if (head <= hdr->nack[i] && hdr->nack[i] <= tail) {
                                sendto_dbg(sock, window[hdr->nack[i] % WINDOW_SIZE],
                                       sizeof(window[hdr->nack[i] % WINDOW_SIZE]), 0,
                                       (struct sockaddr *) &send_addr, sizeof(send_addr));

                                allbytes += 8*strlen(window[hdr->nack[i] % WINDOW_SIZE]);
                                if(allbytes>TMB){
                                    allbytes = allbytes-TMB;
                                    ii_TMB++;}
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
                            bytes = read(fd, data_buf, sizeof(mess_buf) - sizeof(uhdr) -1);
                            data_buf[bytes] = '\0';

                            totalbyte += bytes;
                            allbytes +=bytes;
                            if(allbytes>TMB){
                                allbytes = allbytes -TMB;
                                ii_TMB++;}
                            if(totalbyte > TMB){
                                i_TMB++;
                                totalbyte =  totalbyte -TMB;
                                gettimeofday(&now, NULL);
                                timersub(&now, &TMBtime, &diff_time);
                                gettimeofday(&TMBtime, NULL);
                                printf("Total data I send so far : %lf MB\n",(totalbyte/1000000.0)+i_TMB*10);
                                printf("Average Sending Rate: %lf MB per s\n", 10 / (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
                            }
                            
                            // Check whether Read the end of the file
                            if (bytes < sizeof(mess_buf) - sizeof(uhdr)-1) {
                                printf("Finished Reading .\n");
                                eof = 1;
                            }

                            hdr->seq = seq++;

                            // Store Message in window
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
            /* No Response From Receiver */
            if (head == tail) {
                printf("Finish Sending the file ....\n");
                hdr->seq = -1;
                sendto(sock, mess_buf, sizeof(uhdr) + strlen(data_buf), 0,
                           (struct sockaddr *) &send_addr, sizeof(send_addr));
                gettimeofday(&finishtime, NULL);
                timersub(&finishtime, &initialtime, &diff_time);
                printf("Sending time  %lf s.\n",
                       diff_time.tv_sec + (diff_time.tv_usec / 1000000.0));
                printf("Total Bytes I send : %lf MB\n",totalbyte/1000000.0+i_TMB*10);
                printf("Total Bytes including junk bytes I send : %lf MB\n",allbytes/1000000.0+ii_TMB*10);
                printf("Sending Rate: %lf MB/s", (totalbyte/1000000.0+i_TMB*10)/(diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
                close(fd);
                exit(0);
            }
            /* Time out, Resend everything in buffer to Receiver */
            for (int i = head; i <= tail; i++) {
                // Send Message
                sendto_dbg(sock, window[i % WINDOW_SIZE], sizeof(window[i % WINDOW_SIZE]), 0,
                       (struct sockaddr *) &send_addr, sizeof(send_addr));

                allbytes += 8*strlen(window[i % WINDOW_SIZE]);
                if(allbytes>TMB){
                    allbytes = allbytes-TMB;
                    ii_TMB++;}
            }
        }
    }
    return 0;
}


static void Usage(int argc, char *argv[]) {
    if (argc != 5){
        printf("Usage: ncp <loss_rate> <env> <source_file_name> <dest_file_name>@<ip_address>:<port>\n");
        exit(0);
    }
    Loss_rate = atoi(argv[1]);
    printf("We are in %s network...",argv[2]);
    sendto_dbg_init(Loss_rate);
    Source_file_name = argv[3];
    char* delim = "@:";
    Dest_file_name = strtok(argv[4], delim);
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
