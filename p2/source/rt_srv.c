#include "udp_stream_common.h"
static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static int Cmp_addr(struct sockaddr_in addr1,struct sockaddr_in addr2);
static int Loss_rate;
static int ClientPort;
static int AppPort;

int main(int argc, char *argv[]) {
    int app, rcv;                                /* socket identifier */
    struct sockaddr_in app_addr;                /* address to bind to app */
    struct sockaddr_in rcv_addr;                /* address to bind to receive */
    struct sockaddr_in current_addr;
    socklen_t rcv_len;
    socklen_t app_len;

    fd_set read_mask;
    fd_set mask;

    int seq = 1;                                /* Sequence number of packets */
    int Request = 0;
    int bytes;

    struct stream_pkt app_pkt;
    struct package rcv_pkt;

    int WINDOW_SIZE;
    struct timeval latencyWindow;

    int head = 1;     /* head: index of the first element in the window*/
    int tail = 0;     /* tail: index of the last element in the window*/
    struct timeval Halfrtt = {0, 40};            /* initial 1/2 RTT */
    struct timeval now;
    struct timeval temp;


    Usage(argc, argv);
    /* Open socket for receiving */
    app = socket(AF_INET, SOCK_DGRAM, 0);
    rcv = socket(AF_INET, SOCK_DGRAM, 0);
    if (app < 0 || rcv < 0) {
        perror("app,rcv: socket");
        exit(1);
    }

    /* Bind socket with app and rcv */
    app_addr.sin_family = AF_INET;
    app_addr.sin_addr.s_addr = INADDR_ANY;
    app_addr.sin_port = htons(AppPort);
    rcv_addr.sin_family = AF_INET;
    rcv_addr.sin_addr.s_addr = INADDR_ANY;
    rcv_addr.sin_port = htons(ClientPort);
    if (bind(app, (struct sockaddr *) &app_addr, sizeof(app_addr)) < 0) {
        perror("app: bind");
        exit(1);
    }
    if (bind(rcv, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr)) < 0) {
        perror("rcv: bind");
        exit(1);
    }
    printf("successfully bind the two ports app_port:%d and client_port:%d \n", AppPort, ClientPort);

    /*-----------------Reply Request from receiver------------------------*/
    FD_ZERO(&read_mask);
    FD_SET(rcv, &read_mask);

    printf("Waiting for request of receiver...\n");
    while (!Request) {
        /* Reset the mask */
        mask = read_mask;
        int num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0) {
            /* send the permission */
            if (FD_ISSET(rcv, &mask)) {
                /* receive the request until get it*/
                rcv_len = sizeof(rcv_addr);
                bytes = recvfrom(rcv, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, &rcv_len);
                if (rcv_pkt.type == 3) {
                    /* Get necessary info from Sender*/
                    current_addr = rcv_addr;
                    Request = 1;
                    WINDOW_SIZE = rcv_pkt.WindowSize;
                    latencyWindow = rcv_pkt.LatencyWindow;
                    /* send the permission */
                    rcv_pkt.type = 4;
                    gettimeofday(&rcv_pkt.Receive_TS1, NULL);
                    sendto_dbg(rcv, (char *) &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr,
                               sizeof(rcv_addr));
                }
            }
        }
    }
    printf("successfully connect the receiver\n");

    /* Initialize the window and slide */
    /* slide store the sum of sendTS+LatencyWindow, since they are fixed all the time*/
    struct package *window = malloc(WINDOW_SIZE * sizeof(struct package));
    struct timeval *slide = malloc(WINDOW_SIZE * sizeof(struct timeval));

    /*-------------------------------Main Part-----------------------------*/
    /* Listen messages from local app, Send message to Receiver and Response to Receiver */
    FD_ZERO(&read_mask);
    FD_SET(rcv, &read_mask);
    FD_SET(app, &read_mask);
    for (;;) {
        mask = read_mask;
        int num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0) {
            /* case1, receive app's data and send to sender */
            if (FD_ISSET(app, &mask)) {
                app_len = sizeof(app_addr);
                bytes = recvfrom(app, &app_pkt, sizeof(app_pkt), 0, (struct sockaddr *) &app_addr,app_len);
                tail++;
                /* Send package to receiver */
                printf("%d \n",seq);
                memcpy(&rcv_pkt.data,&app_pkt.data, sizeof(app_pkt.data));
                gettimeofday(&rcv_pkt.Send_TS, NULL);
                rcv_pkt.type = 0;
                rcv_pkt.seq = seq++;
                rcv_pkt.N_Send_TS.tv_sec = -1;
                rcv_pkt.N_Send_TS.tv_usec = -1;
                sendto_dbg(rcv, (char *) &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr));
                /* Store data into window */
                memcpy(&window[tail % WINDOW_SIZE], &rcv_pkt, sizeof(rcv_pkt));
                if (tail - head == WINDOW_SIZE) { head++; }
                /* Store sendTS + LatencyWindow into slide */
                timeradd(&rcv_pkt.Send_TS, &latencyWindow, &slide[tail % WINDOW_SIZE]);
            }

            /* case2, response to receiver */
            if (FD_ISSET(rcv, &mask)) {
                rcv_len = sizeof(rcv_addr);
                bytes = recvfrom(rcv, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, &rcv_len);
                /* Receiver sends ACK (nack, updated RTT)*/
                if (rcv_pkt.type == 2) {
                    /* update 1/2 rtt */
                    Halfrtt = rcv_pkt.Halfrtt;
                    /* send ACKACK to allow rcv calculate the delta and 1/2rtt*/
                    rcv_pkt.type = 1;
                    gettimeofday(&rcv_pkt.ACKACK_TS, NULL);
                    sendto_dbg(rcv, (char *) &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr,
                               sizeof(rcv_addr));
                    /* resend loss package */
                    for (int i = 0; i < NACK_SIZE; i++) {
                        int lossSeq = rcv_pkt.nack[i];
                        /* process NACK*/
                        if (lossSeq != -1 && lossSeq >= head && lossSeq <= tail) {
                            gettimeofday(&now, NULL);
                            /* check time */
                            timeradd(&Halfrtt,&now,&temp);
                            /* sendTS+LatencyWindow -1/2RTT >=now */
                            if (Cmp_time(slide[lossSeq % WINDOW_SIZE], temp) > -1) {
                                memcpy(&rcv_pkt, &window[lossSeq % WINDOW_SIZE], sizeof(rcv_pkt));
                                rcv_pkt.type = 6;
                                gettimeofday(&rcv_pkt.N_Send_TS, NULL);
                                sendto_dbg(rcv, (char *) &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr,
                                           sizeof(rcv_addr));
                            }
                        }
                    }
                }

                /* Receiver sends Request */
                if (rcv_pkt.type == 3) {
                    /*Check the address*/
                    int cmp = Cmp_addr(current_addr, rcv_addr);
                    /* Send decline when the address does not match the current address we are using*/
                    if (cmp == -1) {
                        rcv_pkt.type = 5;
                        gettimeofday(&rcv_pkt.Receive_TS1, NULL);
                        sendto_dbg(rcv, (char *) &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr,
                                   sizeof(rcv_addr));
                    }
                }
            }
        }

        /* case3, Sliding the window based on the Condition: sendTS+1/2RTT+latencyWindow<=Sender_now */
        gettimeofday(&now, NULL);
        for (int i = head; i <= tail; i++) {
            timeradd(&slide[i % WINDOW_SIZE], &Halfrtt, &temp);
            if (Cmp_time(now, temp) > -1) {
                head++;
                puts("Move");
            } else {
                break;
            }
        }
    }
    return 0;
}
static void Usage(int argc, char *argv[]){
    if (argc != 4){Print_help();}
    Loss_rate = atoi(argv[1]);
    sendto_dbg_init(Loss_rate);
    AppPort = atoi(argv[2]);
    ClientPort = atoi(argv[3]);
    printf("Listening for app messages on port %d\n",AppPort);
    printf("Listening for client messages on port %d\n",ClientPort);
}
static void Print_help(){
    printf("Usage:rt_srv <loss_rate_percent> <app_port> <client_port>\n");
    exit(0);
}

/* Returns 1 if t1 > t2, -1 if t1 < t2, 0 if equal */
static int Cmp_time(struct timeval t1, struct timeval t2){
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

/* Returns 1 if they are equal, otherwise return -1 */
static int Cmp_addr(struct sockaddr_in addr1,struct sockaddr_in addr2) {
    if (ntohl(addr1.sin_port) != ntohl(addr2.sin_port)) { return -1; }
    if (ntohl(addr1.sin_addr.s_addr) != ntohl(addr2.sin_addr.s_addr)) { return -1; }
    return 1;
}