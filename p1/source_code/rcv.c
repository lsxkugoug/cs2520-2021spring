#include "net_include.h"

static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static const struct timeval Zero_time = {0, 0};
static int Loss_rate;
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
    struct timeval initialtime = {0,0};
    struct timeval TMBtime ;
    int  initial= 0;
    struct  timeval finishtime = {0,0};
    struct timeval ACKtimeout ={0,0} ;

    uhdr  *temp_hdr;
    char *temp_data_buf;

    int totalbytes = 0;
    int i_TMB = 0;

    /*Message from sender*/
    char mess_buf[MAX_MESS_LEN];
    uhdr  *hdr = (uhdr *)mess_buf;
    char  *data_buf = &mess_buf[sizeof(uhdr)];

    /*Message echo back to sender*/
    char echo_mess_buf[MAX_MESS_LEN];
    uhdr  *echo_hdr = (uhdr *)echo_mess_buf;
    char  *echo_data_buf = &echo_mess_buf[sizeof(uhdr)];

    /* Receiver :  C_ack | Expected:C_ack+1 Buffer Window:[C_ack+2, ....... C_ack+Window_size+1]*/
    char  window[WINDOW_SIZE][MAX_MESS_LEN];

    int C_ack = 0;
    /* If we do not Todo */
    int lastACK = -1;
    int NACKTime =0;

    int buffersize = 0;
    /* in the buffer array, 0 indicates no buffer in the window, 1 indicates not in the window.*/
    int buffer[WINDOW_SIZE] = {0};

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
        printf("%d \n",totalbytes);
        /* (Re)set mask and timeout */
        mask = read_mask;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        gettimeofday(&now, NULL);
        timersub(&now, &ACKtimeout, &diff_time);
        if(diff_time.tv_sec + (diff_time.tv_usec / 1000000.0) >1.8 ){
            echo_hdr->cack = C_ack;
            echo_hdr->nack[0] = C_ack+1;
            for(int i = 1;i<NACK_SIZE;i++){echo_hdr->nack[i] = -1;}
            sendto_dbg(sock, echo_mess_buf, sizeof(uhdr) + strlen(echo_data_buf), 0,
                           (struct sockaddr *) &from_addr,
                           sizeof(from_addr));
            gettimeofday(&ACKtimeout,NULL);
        }

        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);

        if (num > 0)
        {
            if (FD_ISSET(sock, &mask)) {
                from_len = sizeof(from_addr);
                bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,
                                 (struct sockaddr *) &from_addr,
                                 &from_len);
                if(initial == 0){
                    gettimeofday(&initialtime, NULL);
                    initial=1;
                }
                from_ip = from_addr.sin_addr.s_addr;

                /* Record time we received this msg */
                gettimeofday(&last_recv_time, NULL);

                ip1 = (htonl(from_ip) & 0xff000000) >> 24;
                ip2 = (htonl(from_ip) & 0x00ff0000) >> 16;
                ip3 = (htonl(from_ip) & 0x0000ff00) >> 8;
                ip4 = (htonl(from_ip) & 0x000000ff);
                if (have_filename == 0) {
                    /* Use a dummy txt file first then Change to the source file name.
                     * */
                    if ((fw = fopen("dummy.txt", "w")) == NULL) {
                        perror("fopen");
                        exit(0);
                    }
                    have_filename= 1;
                }
                printf( "Receive %d ,C_ACK %d \n ",hdr->seq,C_ack);

                if(hdr->seq==0){
                    /* Receive the name of file */
                    rename("dummy.txt",data_buf);
                }else {
                    //Todo
                    ACKtimeout.tv_usec = 0;
                    ACKtimeout.tv_sec =0;
                    /*Case 1. Message->seq = Cumulative ACK+1
                     *  Receiver get the expected message
                     */
                    if (hdr->seq == C_ack+1) {
                        /* Subcase 1. If no buffer in window.
                         * C_ack ++, write data_buff into file, send Cumulative ACK to sender
                         */
                        if (buffersize == 0) {
                            C_ack++;

                            /*Todo*/
                            lastACK = C_ack ;
                            NACKTime = 0;
                            /*lsx*/
                            totalbytes += 8*(sizeof(mess_buf)-sizeof(uhdr));
                            if(totalbytes > TMB){
                                totalbytes = totalbytes -TMB;
                                i_TMB++;
                                gettimeofday(&now, NULL);
                                timersub(&now, &TMBtime, &diff_time);
                                gettimeofday(&TMBtime, NULL);
                                printf("Total data I Received so far : %lf MB\n",(totalbytes/1000000.0)+i_TMB*10);
                                printf("Average Receiving Rate: %lf MB per s\n", 10 / (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
                            }

                            /* Write Data into File*/
                            fprintf(fw, "%s", data_buf);
                            bzero(data_buf, sizeof(data_buf));

                            /* Fill in header info */
                            echo_hdr->cack = C_ack;
                            for (int i = 0; i < NACK_SIZE; i++) { echo_hdr->nack[i] = -1; } //-1 indicates no missing packets.

                            /* Send Message to sender */
                            sendto_dbg(sock, echo_mess_buf, sizeof(uhdr) + strlen(echo_data_buf), 0,
                                   (struct sockaddr *) &from_addr, sizeof(from_addr));
                        } else {
                            /* Subcase 2. If there exist buffer in window
                             * Find the Largest C_ack, then write all buffered data. send Cumulative ACK to sender
                             * Receiver :  C_ack | Expected:C_ack+1 Buffer Window:[C_ack+2, ....... C_ack+Window_size+1]
                             */

                            totalbytes += 8*(sizeof(mess_buf)-sizeof(uhdr));
                            if(totalbytes > TMB){
                                totalbytes = totalbytes -TMB;
                                i_TMB++;
                                gettimeofday(&now, NULL);
                                timersub(&now, &TMBtime, &diff_time);
                                gettimeofday(&TMBtime, NULL);
                                printf("Total data I received so far : %lf MB\n",(totalbytes/1000000.0)+i_TMB*10);
                                printf("Average Receiving Rate: %lf MB per s\n", 10 / (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
                            }
                            fprintf(fw, "%s", data_buf);
                            bzero(data_buf, sizeof(data_buf));

                            int temp = C_ack;
                            C_ack ++;
                            /* Search the Buffer window Find the largest ACK */
                            for(int j = temp+2; j<(temp+1+WINDOW_SIZE) && buffersize>0 ;j++){
                                if(buffer[j%WINDOW_SIZE]==1){
                                    C_ack++;
                                    buffer[j%WINDOW_SIZE] = 0;
                                    buffersize--;

                                    temp_hdr = (uhdr *) window[j % WINDOW_SIZE];
                                    temp_data_buf = &window[j % WINDOW_SIZE][sizeof(uhdr)];

                                    totalbytes += 8*(sizeof(mess_buf)-sizeof(uhdr));
                                    if(totalbytes > TMB){
                                        totalbytes = totalbytes- TMB;
                                        i_TMB++;
                                        gettimeofday(&now, NULL);
                                        timersub(&now, &TMBtime, &diff_time);
                                        gettimeofday(&TMBtime, NULL);
                                        printf("Total data I Received so far : %lf MB\n",(totalbytes/1000000.0)+i_TMB*10);
                                        printf("Average Receiving Rate: %lf MB per s\n", 10 / (diff_time.tv_sec + (diff_time.tv_usec / 1000000.0)));
                                    }
                                    fprintf(fw, "%s", temp_data_buf);
                                    bzero(temp_data_buf, sizeof(temp_data_buf));

                                }else{
                                    break;
                                }
                            }
                            /* Todo*/
                            lastACK = C_ack;
                            NACKTime = 0;

                            /* Fill in echo header*/
                            echo_hdr->cack = C_ack;
                            for (int i = 0; i < NACK_SIZE; i++) { echo_hdr->nack[i] = -1; } //-1 indicates no missing packets.

                            /* Send Message to sender */
                            sendto_dbg(sock, echo_mess_buf, sizeof(uhdr) + strlen(echo_data_buf), 0,
                                   (struct sockaddr *) &from_addr,
                                   sizeof(from_addr));
                        }
                    }
                    else {
                        /* Case 2. Message->seq != Cumulative ACK +1
                         *  Receiver does not get the expected message
                         */
                        /*
                         * Subcase 1.Message->seq < Cumulative ACK+1
                         *  Delayed message, drop it.
                         *  or Message->seq> Cumulative ACK +Window Size
                         *  Drop it
                         * Receiver :  C_ack | Expected:C_ack+1 Buffer Window:[C_ack+2, ....... C_ack+Window_size+1]
                         */
                        if(hdr->seq<C_ack+1){}
                        if(hdr->seq>C_ack+WINDOW_SIZE+1){}
                        /* Subcase 2.Message->seq > Cumulative ACK+1
                         * If seq in buffer, then drop it
                         *  Otherwise, put data into buffer, search for the last 5 missing packet and put them into Nack array.
                         *  Then Send the message to sender
                         *  Receiver :  C_ack | Expected:C_ack+1 Buffer Window:[C_ack+2, ....... C_ack+Window_size+1]
                         */
                        if (hdr->seq >= (C_ack + 2) && hdr->seq <= (C_ack + 1 + WINDOW_SIZE)) {
                            if (buffer[hdr->seq % WINDOW_SIZE] == 0) {

                                buffer[hdr->seq % WINDOW_SIZE] = 1;
                                buffersize++;

                                /* Store message in window */
                                memcpy(window[hdr->seq % WINDOW_SIZE],mess_buf, sizeof(mess_buf));

                                /* Todo*/
                                if( lastACK != C_ack || (lastACK == C_ack && NACKTime<2) ) {
                                    if(lastACK == C_ack) {
                                        NACKTime++;
                                    } else{
                                        NACKTime = 0;
                                        lastACK = C_ack;
                                    }

                                    /* Fill in the header */
                                    for (int j = 0; j < NACK_SIZE; j++) { echo_hdr->nack[j] = -1; }

                                    /* First ask for the one we expect which is C_ack+1*/
                                    echo_hdr->nack[0] = C_ack + 1;

                                    /* Then search in buffer and find the missing packet
                                     * and send Nack */
                                    for (int j = C_ack+2, i = 1; (j < hdr->seq) && (i < NACK_SIZE); i++) {
                                        if (buffer[j % WINDOW_SIZE] == 0) {
                                            echo_hdr->nack[i] = j;
                                        }
                                        j++;
                                    }

                                    echo_hdr->cack = C_ack;

                                    /* Send message to sender*/
                                    sendto_dbg(sock, echo_mess_buf, sizeof(uhdr) + strlen(echo_data_buf), 0,
                                               (struct sockaddr *) &from_addr,
                                               sizeof(from_addr));
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {          
            // printf("timeout...nothing received for 100 microseconds.\n");
            gettimeofday(&now, NULL);
            if (Cmp_time(last_recv_time, Zero_time) > 0)
            {
                timersub(&now, &last_recv_time, &diff_time);
              //  printf("last msg received %lf seconds ago.\n\n",
                //       diff_time.tv_sec + (diff_time.tv_usec / 1000000.0));
                if(diff_time.tv_sec>=5){
                    gettimeofday(&finishtime, NULL);
                    timersub(&finishtime, &initialtime, &diff_time);
                    printf("Total Receiving time  %lf s.\n",
                           diff_time.tv_sec-5 + (diff_time.tv_usec / 1000000.0));
                    printf("Total data I  Received : %lf MB\n",totalbytes/1000000.0+10*i_TMB);
                    printf("Total Receiving Rate: %lf MB/s", ((totalbytes/1000000.0)+10*i_TMB)/(diff_time.tv_sec-5 + (diff_time.tv_usec / 1000000.0)));
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
/* rcv <loss_rate_percent> <port> <env> */
static void Usage(int argc, char *argv[])
{
    if (argc != 4)
    {
        Print_help();
    }
    Loss_rate = atoi(argv[1]);
    sendto_dbg_init(Loss_rate);
    if (sscanf(argv[2], "%d", &Port) != 1)
    {
        Print_help();
    }
    printf("We are in %s network...",argv[3]);
}

static void Print_help()
{
    printf("Usage: udp_server <loss_rate_percent> <port> <env> \n");
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




