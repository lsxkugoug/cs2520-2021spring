#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static void Store_Message_in_Window(char Message[], int index, char window[WINDOW_SIZE][MAX_MESS_LEN]);
static const struct timeval Zero_time = {0, 0};

static int Port;

int main(int argc, char *argv[])
{
    struct sockaddr_in name;
    struct sockaddr_in from_addr;
    socklen_t from_len;
    int from_ip;
    int sock;
    fd_set mask;
    fd_set read_mask;
    int bytes;
    int num;
    struct timeval timeout;
    struct timeval last_recv_time = {0, 0};
    struct timeval now;
    struct timeval diff_time;

    uhdr  *temp_hdr;
    char *temp_data_buf;

    /*Message from sender*/
    char mess_buf[MAX_MESS_LEN];
    uhdr  *hdr = (uhdr *)mess_buf;
    char  *data_buf = &mess_buf[sizeof(uhdr)];

    /*Message echo back to sender*/
    char echo_mess_buf[MAX_MESS_LEN];
    uhdr  *echo_hdr = (uhdr *)echo_mess_buf;
    char  *echo_data_buf = &echo_mess_buf[sizeof(uhdr)];

    char  window[WINDOW_SIZE][MAX_MESS_LEN]; // Start from Cumulative ACK +2 to Cumulative ACK+1+window size

    int C_ack = 0;
    int buffersize = 0;
    int buffer[WINDOW_SIZE] = {0};
    // in the buffer array, 0 indicates no buffer in the window, 1 indicates not in the window.
    FILE *fw; /* Pointer to dest file, which we write  */
    int nwritten;

    int ip1,ip2,ip3,ip4;

    /* Parse commandline args */
    Usage(argc, argv);
    printf("Listening for messages on port %d\n", Port);

    /* Open socket for receiving */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("udp_server: socket");
        exit(1);
    }

    /* Bind receive socket to listen for incoming messages on specified port */
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(Port);

    if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        perror("udp_server: bind");
        exit(1);
    }

    /* Set up mask for file descriptors we want to read from */
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    int have_filename = 0;

    for (;;)
    {
        /* (Re)set mask and timeout */
        mask = read_mask;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if (num > 0)
        {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,
                                 (struct sockaddr *) &from_addr,
                                 &from_len);

                from_ip = from_addr.sin_addr.s_addr;

                /* Record time we received this msg */
                gettimeofday(&last_recv_time, NULL);

                printf("C_ack:%d, Seq:%d\n", C_ack,hdr->seq);

                ip1 = (htonl(from_ip) & 0xff000000) >> 24;
                ip2 = (htonl(from_ip) & 0x00ff0000) >> 16;
                ip3 = (htonl(from_ip) & 0x0000ff00) >> 8;
                ip4 = (htonl(from_ip) & 0x000000ff);

                //printf("Received from (%d.%d.%d.%d): %s\n", ip1, ip2, ip3, ip4, data_buf);

                if (have_filename == 0) {
                    /* Use IP+Port to form a unique file name
                     * for example 1.2.3.4:5000.txt
                     * */
                    char portstr[5];
                    sprintf(portstr, "%d", Port);
                    char filename[30];
                    for (int i = 1; i < 6; i = i + 2) {
                        filename[i] = '.';
                    }
                    filename[0] = '0' + ip1;
                    filename[2] = '0' + ip2;
                    filename[4] = '0' + ip3;
                    filename[6] = '0' + ip4;
                    filename[7] = ':';
                    strcat(filename, portstr);

                    char extension[] = ".txt";
                    strcat(filename, extension);

                    if ((fw = fopen("czy.txt", "w")) == NULL) {
                        perror("fopen");
                        exit(0);
                    }
                    have_filename= 1;
                }

                if(hdr->seq==0){
                    /*Receive the name of file*/
                    // Todo change the file name
                }else {

                    /*Case 1. Message->seq = Cumulative ACK+1
                     *  Receiver get the expected message
                     */
                    if (hdr->seq == C_ack + 1) {
                        /* Subcase 1. If no buffer in window.
                         * C_ack ++, write data_buff into file, send Cumulative ACK to sender
                         *  Receiver :  C_ack ( window:) C_ack+1 [C_ack+2, ....... C_ack+Window_size]
                         */

                        if (buffersize == 0) {
                            puts("S1");
                            C_ack++;
                            fprintf(fw, "%s", data_buf);
                            bzero(data_buf, sizeof(data_buf));

                            /* Fill in header info */
                            echo_hdr->cack = C_ack;
                            for (int i = 0; i < NACK_SIZE; i++) { echo_hdr->nack[i] = -1; } //-1 indicates no missing packets.

                            /* Send Message to sender */
                            sendto(sock, echo_mess_buf, sizeof(uhdr) + strlen(echo_data_buf), 0,
                                   (struct sockaddr *) &from_addr, sizeof(from_addr));
                        } else {
                            /* Subcase 2. If there exist buffer in window
                             * Find the Largest C_ack, then write all buffered data. send Cumulative ACK to sender
                             * Receiver :  C_ack ( window:) C_ack+1[C_ack+1+1, ....... C_ack+1+Window_size]
                             */
                            puts("S2");
                            fprintf(fw, "%s", data_buf);
                            bzero(data_buf, sizeof(data_buf));
                            int temp = C_ack;
                            C_ack = C_ack +1;

                            for(int j = temp+2;j<(temp+1+WINDOW_SIZE);j++){
                                if(buffer[j%WINDOW_SIZE]==1){
                                    C_ack++;
                                    buffer[j%WINDOW_SIZE] = 0;
                                    buffersize--;

                                    temp_hdr = (uhdr *) window[j % WINDOW_SIZE];
                                    temp_data_buf = &window[j % WINDOW_SIZE][sizeof(uhdr)];

                                    fprintf(fw, "%s", temp_data_buf);
                                    bzero(temp_data_buf, sizeof(temp_data_buf));
                                }else{
                                    break;
                                }
                            }

                            /* Fill in echo header*/
                            echo_hdr->cack = C_ack;
                            for (int i = 0; i < NACK_SIZE; i++) { echo_hdr->nack[i] = -1; } //-1 indicates no missing packets.

                            /* Send Message to sender */
                            sendto(sock, echo_mess_buf, sizeof(uhdr) + strlen(echo_data_buf), 0,
                                   (struct sockaddr *) &from_addr,
                                   sizeof(from_addr));
                        }
                    }
                    else {
                        /* Case 2. Message->seq != Cumulative ACK
                         *  Receiver does not get the expected message
                         */
                        /*
                         * Subcase 1.Message->seq < Cumulative ACK+1
                         *  Delayed message, drop it.
                         *  or Message->seq> Cumulative ACK +Window Size
                         *  Drop it
                         */
                        if(hdr->seq<C_ack+1){
                            puts("S3");
                        }

                        /*C_ack ( window:) C_ack+1 [ C_ack+2, ....... C_ack+Window_size]*/
                        /* Subcase 2.Message->seq > Cumulative ACK+1
                         * If seq in buffer, then drop it
                         * Otherwise, put data into buffer, search for the last 5 missing packet and put them into Nack array.
                         *  Then Send the message to sender
                         */
                        if (hdr->seq > (C_ack + 1) && hdr->seq < (C_ack + 1 + WINDOW_SIZE)) {
                            puts("S4");
                            if (buffer[hdr->seq % WINDOW_SIZE] == 0) {
                                buffer[hdr->seq % WINDOW_SIZE] = 1;
                                buffersize++;

                                /* Store message in window */
                                Store_Message_in_Window(mess_buf, hdr->seq, window);

                                /* Fill in the header */
                                for (int j = 0; j < NACK_SIZE; j++) { echo_hdr->nack[j] = -1; }
                                for (int j = hdr->seq-1, i = 0; (j>C_ack+1 )&& (i<NACK_SIZE); i++){
                                    if(buffer[j%WINDOW_SIZE] ==0){
                                        printf("%d ",j);
                                        echo_hdr->nack[i] = j;
                                    }
                                    j--;
                                }

                                echo_hdr->cack = C_ack;

                                /* Send message to sender*/
                                sendto(sock, echo_mess_buf, sizeof(uhdr) + strlen(echo_data_buf), 0,
                                       (struct sockaddr *) &from_addr,
                                       sizeof(from_addr));
                            }
                        }
                    }
                }
            }
        }
        else
        {          
            printf("timeout...nothing received for 10 seconds.\n");
            gettimeofday(&now, NULL);
            if (Cmp_time(last_recv_time, Zero_time) > 0)
            {
                timersub(&now, &last_recv_time, &diff_time);
                printf("last msg received %lf seconds ago.\n\n",
                       diff_time.tv_sec + (diff_time.tv_usec / 1000000.0));
                if(diff_time.tv_sec>20){
                    fclose(fw);
                    return 0;
                }
            }
        }
    }
    fclose(fw);
    return 0;
}

/* Read commandline arguments */
static void Usage(int argc, char *argv[])
{
    if (argc != 2)
    {
        Print_help();
    }

    if (sscanf(argv[1], "%d", &Port) != 1)
    {
        Print_help();
    }
}

static void Print_help()
{
    printf("Usage: udp_server <port>\n");
    exit(0);
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

/* Store the message into Window */
static void Store_Message_in_Window(char Message[], int index, char window[WINDOW_SIZE][MAX_MESS_LEN]){
    // Calculate the index for window
    int index_window = index%WINDOW_SIZE;
    memcpy(window[index_window],Message, sizeof(Message));
}



