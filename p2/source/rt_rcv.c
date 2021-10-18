#include "udp_stream_common.h"
static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static struct timeval Half_time(struct  timeval t);
static const struct timeval Zero_time = {0, 0};
static const struct timeval Report_Interval = {5, 0};
static struct timeval Base_Delta ;
static struct timeval Half_RTT = {0,40000};
static int Latency_Window_time ;
static struct timeval Latency_Window;
static int WINDOW_SIZE;
static int Loss_rate;
static int ServerPort;
static int AppPort;
static char *ServerIP;
/* Receiver Process */

int main(int argc, char *argv[]) {
    struct sockaddr_in Receiver_addr;
    struct sockaddr_in Sender_addr;
    struct sockaddr_in Localapp_addr;
    socklen_t Sender_len;

    struct hostent h_ent;
    struct hostent *p_h_ent;
    int host_num;
    struct hostent Local_h_ent;
    struct hostent *Local_p_h_ent;
    int Local_host_num;
    char localIP[] = "127.0.0.1";

    int sock;
    fd_set mask;
    fd_set read_mask;
    int bytes;
    int num;
    int Getpermission=0;

    struct timeval now;
    struct timeval start_ts;
    struct timeval timeout;
    struct timeval next_report_time;
    struct timeval *to_ptr;
    struct timeval diff_time;
    struct timeval deliver_time;
    static struct timeval base_delta ;
    static struct timeval half_rtt ;
    struct timeval temp1;
    struct timeval temp2;
    long int duration;

    /* Packet to UPD*/
    struct stream_pkt send_pkt;

    /* Packet echo back to Sender*/
    struct package echo_pkt;
    /* Packet from sender */
    struct package sender_pkt;

    struct package temp_pkt;

    /* Latest Record of Delta and RTT */
    int32_t delta_sec[RECORD_SIZE]={0};
    int32_t delta_usec[RECORD_SIZE] = {0};
    int32_t delta_i = 0;
    int32_t rtt_sec[RECORD_SIZE]={0};
    int32_t rtt_usec[RECORD_SIZE] = {0};
    int32_t rtt_i = 0;

    int C_ack = 0;
    /* Report parameter*/
    int rcvd_count = 0;
    int highestseq = 0;
    int loss;
    double lossrate;
    double rate;
    double avg_oneway;
    double max_oneway;
    double min_oneway;
    double oneway;
    /*--------------Main Part of Program--------------------*/
    Usage(argc,argv);
    /* Initialization of Latency Window */
    struct package *window = malloc(sizeof(struct package) *WINDOW_SIZE);
    int buffersize = 0;
    int buffer[WINDOW_SIZE];
    memset(buffer,0,WINDOW_SIZE* sizeof(int));

    /* Open socket for receiving and delivering */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0){perror("udp_server: socket");exit(1);}

    /* Convert Sender IP address to format we need */
    p_h_ent = gethostbyname(ServerIP);
    if (p_h_ent == NULL) {printf("Receiver: Server IP gethostbyname error.\n");exit(1);}
    memcpy(&h_ent, p_h_ent, sizeof(h_ent));
    memcpy(&host_num, h_ent.h_addr_list[0], sizeof(host_num));
    Sender_addr.sin_family = AF_INET;
    Sender_addr.sin_addr.s_addr = host_num;
    Sender_addr.sin_port = htons(ServerPort);

    /*  Create instance of Local APP address */
    Local_p_h_ent = gethostbyname(localIP);
    if(Local_p_h_ent == NULL){puts("Local ip error!");exit(1);}
    memcpy(&Local_h_ent, Local_p_h_ent, sizeof(Local_h_ent));
    memcpy(&Local_host_num, h_ent.h_addr_list[0], sizeof(host_num));
    Localapp_addr.sin_family = AF_INET;
    Localapp_addr.sin_addr.s_addr = Local_host_num;
    Localapp_addr.sin_port = htons(AppPort);

    /* Set up mask for sock */
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    /*--------------Connection Part--------------------*/
    /* Keep Sending request every 1s until we get reply from sender  */
    puts("Sending Request");
    while(!Getpermission){
        mask = read_mask;
        timeout.tv_sec =1;
        timeout.tv_usec = 0;
        num = select(FD_SETSIZE, &mask, NULL, NULL, &timeout);
        if(num>0){
            if (FD_ISSET(sock, &mask)) {
                Sender_len = sizeof(Sender_addr);
                bytes = recvfrom(sock, &sender_pkt, sizeof(sender_pkt), 0, (struct sockaddr *) &Sender_addr,
                                 &Sender_len);
                /* Sender sends decline */
                if (sender_pkt.type == 5) {
                    puts(" Server is being occupied by another Client!");
                    exit(0);
                }
                /* Sender sends the permission.*/
                /* Basically, We could calculate initial RTT and Base Delta based on the record we have */
                if (sender_pkt.type == 4) {
                    Getpermission = 1;
                    gettimeofday(&now, NULL);
                    timersub(&now, &sender_pkt.Receive_TS1, &Base_Delta);
                    timersub(&now, &sender_pkt.Send_TS, &diff_time);
                    Half_RTT = Half_time(diff_time);
                    rtt_sec[rtt_i] = Half_RTT.tv_sec;
                    rtt_usec[rtt_i] = Half_RTT.tv_usec;
                    delta_sec[delta_i] = Base_Delta.tv_sec;
                    delta_usec[delta_i] = Base_Delta.tv_usec;
                    rtt_i++;
                    delta_i++;
                    puts("Successfully connected with Sender!");
                    //TODO
                    printf("%ld,%d. %ld,%d\n",Half_RTT.tv_sec,Half_RTT.tv_usec,Base_Delta.tv_sec,Base_Delta.tv_usec);

                }
            }
        }else{
            gettimeofday(&echo_pkt.Send_TS,NULL);
            echo_pkt.type = 3;
            echo_pkt.WindowSize = WINDOW_SIZE;
            timeradd(&Latency_Window,&Zero_time,&echo_pkt.LatencyWindow);
            sendto_dbg(sock, (char *)&echo_pkt, sizeof(echo_pkt), 0,
                       (struct sockaddr *) &Sender_addr, sizeof(Sender_addr));
        }
    }

    for(;;){
        /*-------Look up the window,and deliver the packet on Delivery Time.-------*/
        gettimeofday(&now,NULL);
        for(int i = C_ack+1; i<C_ack+1+WINDOW_SIZE && buffersize>0 ;i++){
            /* Stop case 1: Stop deliver when we do not have anything in window */
            /*If we miss some packets, we will still check the next packet, for example , 12 456*/
            if(buffer[i%WINDOW_SIZE]==0){continue;}
            temp_pkt = window[i%WINDOW_SIZE];
            /* Now equal to or above sendTS+base_delta+latency window We should deliver the data */
            timeradd(&Base_Delta, &temp_pkt.Send_TS, &deliver_time);
            timeradd(&deliver_time,&Latency_Window,&deliver_time);
            int cmp = Cmp_time(now,deliver_time);
            if(cmp>-1 ){
                send_pkt.seq = temp_pkt.seq;
                send_pkt.ts_sec = now.tv_sec;
                send_pkt.ts_usec = now.tv_usec;
                memcpy(send_pkt.data,temp_pkt.data,sizeof(temp_pkt.data));
                sendto(sock, (char *)&send_pkt, sizeof(send_pkt), 0, (struct sockaddr *) &Localapp_addr, sizeof(Localapp_addr));
                buffersize--;
                buffer[i%WINDOW_SIZE] =0;
                C_ack = i;
            }else{
                /*Stop case 2: Stop deliver when we meet first packet whose delivery time is behind the present time*/
                break;
            }
        }

        /* Receive packets from sender */
        mask = read_mask;
        /* reset timeout. only need to report if we've actually received something already */
        if (rcvd_count == 0) {to_ptr = NULL;}
        else {timersub(&next_report_time, &now, &timeout); to_ptr = &timeout;}

        num = select(FD_SETSIZE, &mask, NULL, NULL, to_ptr);
        if(num>0){
            if (FD_ISSET(sock, &mask)) {
                bytes = recvfrom(sock, &sender_pkt, sizeof(sender_pkt), 0,(struct sockaddr *) &Sender_addr,&Sender_len);
                /* Case 1: Sender sends data packets to Receiver */
                if( sender_pkt.type == 0|| sender_pkt.type==6 ){
                    /* The receiver write it into window
                     * Echo back the message with type 2 and Receive_TS1
                     * and first three Nack , Send ACK, ACK = seq
                     * and Updated 1/2 RTT
                     * */
                    if((sender_pkt.seq>=C_ack+1) && (sender_pkt.seq<C_ack+1+WINDOW_SIZE) && buffer[sender_pkt.seq % WINDOW_SIZE]==0) {
                        /* Check whether the sendTS match the requirement: sendTS+ Base_Delta+ latencyWindow >= now */
                        timeradd(&sender_pkt.Send_TS, &Base_Delta, &temp1);
                        timeradd(&temp1, &Latency_Window, &temp2);
                        gettimeofday(&now, NULL);
                        int k = Cmp_time(temp2, now);
                        if (k > -1) {
                            int32_t t1, t2, t3;
                            /* Update the baseDelta*/
                            gettimeofday(&now, NULL);
                            gettimeofday(&sender_pkt.Receive_TS1, NULL);
                            if (sender_pkt.type == 0) {
                                /* If first packet, start timing */
                                if (rcvd_count == 0) {
                                    gettimeofday(&start_ts, NULL);
                                    timeradd(&start_ts, &Report_Interval, &next_report_time);
                                }
                                if(sender_pkt.seq>highestseq){highestseq = sender_pkt.seq;}
                                /* Calculate oneway delay */
                                oneway = now.tv_sec - sender_pkt.Send_TS.tv_sec;
                                oneway *= 1000;
                                oneway += (now.tv_usec - sender_pkt.Send_TS.tv_usec) / 1000.0;
                                timersub(&now, &sender_pkt.Send_TS, &base_delta);
                            } else {
                                /* Calculate oneway delay */
                                oneway = now.tv_sec - sender_pkt.N_Send_TS.tv_sec;
                                oneway *= 1000;
                                oneway += (now.tv_usec - sender_pkt.N_Send_TS.tv_usec) / 1000.0;
                                timersub(&now, &sender_pkt.N_Send_TS, &base_delta);
                            }
                            if (oneway > max_oneway || rcvd_count == 1) {max_oneway = oneway;}
                            if (oneway < min_oneway || rcvd_count == 1) {min_oneway = oneway;}
                            avg_oneway= (avg_oneway*rcvd_count+oneway)/(rcvd_count+1);
                            rcvd_count++;
                            if (delta_i < RECORD_SIZE) {
                                delta_sec[delta_i] = base_delta.tv_sec;
                                delta_usec[delta_i] = base_delta.tv_usec;
                                t1 = (Base_Delta.tv_sec * delta_i + base_delta.tv_sec) / (delta_i + 1);
                                t2 = (Base_Delta.tv_sec * delta_i + base_delta.tv_sec) - t1 * (delta_i + 1);
                                t3 = (t2 * 1000000 + Base_Delta.tv_usec * delta_i + base_delta.tv_usec) /
                                     (delta_i + 1);
                                Base_Delta.tv_sec = t1;
                                Base_Delta.tv_usec = t3;
                                delta_i++;
                            }
                            else {
                                t1 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE]) / RECORD_SIZE;
                                t2 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE]) - t1 * RECORD_SIZE;
                                t3 = (t2 * 1000000 + base_delta.tv_usec - delta_usec[delta_i % RECORD_SIZE]) /
                                     RECORD_SIZE;
                                Base_Delta.tv_sec += t1;
                                Base_Delta.tv_usec += t3;
                                delta_sec[delta_i % RECORD_SIZE] = base_delta.tv_sec;
                                delta_usec[delta_i % RECORD_SIZE] = base_delta.tv_usec;
                                delta_i++;
                            }

                            /* Put received packet into window */
                            memcpy(&window[sender_pkt.seq % WINDOW_SIZE], &sender_pkt, sizeof(sender_pkt));
                            buffersize++;
                            buffer[sender_pkt.seq % WINDOW_SIZE] = 1;

                            /* Fill in the echo message */
                            echo_pkt.type = 2;
                            timeradd(&Zero_time, &sender_pkt.Send_TS, &echo_pkt.Send_TS);
                            timeradd(&Zero_time, &sender_pkt.N_Send_TS, &echo_pkt.N_Send_TS);
                            gettimeofday(&echo_pkt.Receive_TS1, NULL);
                            echo_pkt.seq = sender_pkt.seq;
                            echo_pkt.ack = sender_pkt.seq;
                            for (int i = 0; i < NACK_SIZE; i++) { echo_pkt.nack[i] = -1; }
                            /* Find at most Nacksize Nacks */
                            for (int i = C_ack + 1, j = 0; i < sender_pkt.seq && j < NACK_SIZE; i++) {
                                if (buffer[i%WINDOW_SIZE] == 0) {
                                    echo_pkt.nack[j] = i;
                                    j++;
                                }
                            }
                            echo_pkt.Halfrtt.tv_sec = Half_RTT.tv_sec;
                            echo_pkt.Halfrtt.tv_usec = Half_RTT.tv_usec;

                            sendto_dbg(sock, (char *) &echo_pkt, sizeof(echo_pkt), 0,
                                       (struct sockaddr *) &Sender_addr, sizeof(Sender_addr));
                        }
                    }
                }
                /* Case 2: Sender sends ACKACK to Receiver (Update the base_delta and halfRTT) */
                if(sender_pkt.type == 1){
                    int32_t t1,t2,t3;
                    /* The receiver gets ACK ACK and update Base Delta and Half_RTT */
                    gettimeofday(&now,NULL);
                    /* Update Base Delta */
                    timersub(&now,&sender_pkt.ACKACK_TS,&base_delta);
                    if(delta_i<RECORD_SIZE) {
                        delta_sec[delta_i] = base_delta.tv_sec;
                        delta_usec[delta_i] = base_delta.tv_usec;
                        t1 = (Base_Delta.tv_sec*delta_i+base_delta.tv_sec)/(delta_i+1);
                        t2 = (Base_Delta.tv_sec*delta_i+base_delta.tv_sec) - t1*(delta_i+1);
                        t3 = (t2*1000000 + Base_Delta.tv_usec*delta_i+base_delta.tv_usec) /(delta_i+1);
                        Base_Delta.tv_sec = t1;
                        Base_Delta.tv_usec = t3;
                        delta_i++;
                    }
                    else{
                        t1 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE])/RECORD_SIZE;
                        t2 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE]) - t1*RECORD_SIZE;
                        t3 = (t2*1000000 + base_delta.tv_usec - delta_usec[delta_i % RECORD_SIZE])/RECORD_SIZE;
                        Base_Delta.tv_sec += t1;
                        Base_Delta.tv_usec+= t3;
                        delta_sec[delta_i % RECORD_SIZE] = base_delta.tv_sec;
                        delta_usec[delta_i % RECORD_SIZE] = base_delta.tv_usec;
                        delta_i++;
                    }

                    /* Update Half of RTT */
                    timersub(&now,&sender_pkt.Receive_TS1,&temp1);// RTT
                    if(sender_pkt.N_Send_TS.tv_sec==-1) {
                        timersub(&sender_pkt.ACKACK_TS, &sender_pkt.Send_TS, &temp2);//RTT
                    }else{
                        timersub(&sender_pkt.ACKACK_TS, &sender_pkt.N_Send_TS, &temp2);//RTT
                    }
                    timeradd(&temp1,&temp2,&diff_time);
                    temp1 = Half_time(diff_time);
                    half_rtt = Half_time(temp1);
                    if(rtt_i<RECORD_SIZE) {
                        if(rtt_i == 0){
                            Half_RTT.tv_sec = half_rtt.tv_sec;
                            Half_RTT.tv_usec = half_rtt.tv_usec;
                            rtt_sec[rtt_i] = half_rtt.tv_sec;
                            rtt_usec[rtt_i] = half_rtt.tv_usec;
                            rtt_i++;
                        }
                        else {
                            rtt_sec[rtt_i] = half_rtt.tv_sec;
                            rtt_usec[rtt_i] = half_rtt.tv_usec;
                            t1 = (Half_RTT.tv_sec * rtt_i + half_rtt.tv_sec) / (rtt_i + 1);
                            t2 = (Half_RTT.tv_sec * rtt_i + half_rtt.tv_sec) - t1 * (rtt_i + 1);
                            t3 = (t2 * 1000000 + Half_RTT.tv_usec * rtt_i + half_rtt.tv_usec) / (rtt_i + 1);
                            Half_RTT.tv_sec = t1;
                            Half_RTT.tv_usec = t3;
                            rtt_i++;
                        }
                    }else {
                        t1 = (half_rtt.tv_sec - rtt_sec[rtt_i % RECORD_SIZE]) / RECORD_SIZE;
                        t2 = (half_rtt.tv_sec - rtt_sec[rtt_i % RECORD_SIZE]) - t1 * RECORD_SIZE;
                        t3 = (t2 * 1000000 + half_rtt.tv_usec - rtt_usec[rtt_i % RECORD_SIZE]) / RECORD_SIZE;
                        Half_RTT.tv_sec += t1;
                        Half_RTT.tv_usec+= t3;
                        rtt_sec[rtt_i % RECORD_SIZE] = half_rtt.tv_sec;
                        rtt_usec[rtt_i % RECORD_SIZE] = half_rtt.tv_usec;
                        rtt_i++;
                    }
                }
                /* Case 3: Other type of message does not make sense , just discard.*/
            }
        }
        else {
            /* timeout */
            /* calculate current rate */
            gettimeofday(&now, NULL);
            duration = now.tv_sec - start_ts.tv_sec;
            duration *= 1000000;
            duration += now.tv_usec - start_ts.tv_usec;
            rate = MAX_DATA_LEN * rcvd_count*8; /* bits sent so far */
            rate = rate / duration; /* bits per usec == megabits per sec */
            loss = C_ack-(rcvd_count-buffersize);
            lossrate = 1.0*100*loss /C_ack;
            /* report */
            printf("%lf sec elapsed\n", duration / 1000000.0);
            printf("%.2f total magabytes recvd,", rcvd_count*MAX_DATA_LEN/1000000.0);
            printf("%d total pkts recvd\n", rcvd_count);
            printf("avg rate: %lf Mbps", rate);
            printf("avg rate: %lf Pps\n",rcvd_count*1000000.0/duration);
            printf("The sequence number of the highest packet received (froom sender) %d so far\n",highestseq);
            printf("The sequence number of the highest packet delivered(to udp receiver) %d so far\n",C_ack);
            printf("The total number of packets lost %d\n",loss);
            printf("Loss rate %lf%%\n",lossrate);
            printf("%lf ms max oneway delay\n", max_oneway);
            printf("%lf ms min oneway delay\n", min_oneway);
            printf("%lf ms avg oneway delay\n\n", avg_oneway);
            timeradd(&next_report_time, &Report_Interval, &next_report_time);
        }
    }
    return 0;
}

static void Usage(int argc, char *argv[]){
    if (argc != 5){Print_help();}
    Loss_rate = atoi(argv[1]);
    sendto_dbg_init(Loss_rate);
    char* delim = ":";
    ServerIP = strtok(argv[2],delim);
    ServerPort = atoi(strtok(NULL,delim));
    AppPort = atoi(argv[3]);
    Latency_Window_time = atoi(argv[4]);
    Latency_Window.tv_sec = Latency_Window_time/1000;
    Latency_Window.tv_usec = (Latency_Window_time- Latency_Window.tv_sec*1000)*1000;
    /*T(ns) * 2.5M bytes /s / 1400 bytes = T *2.5*10^3/ 1400 = T*25/14 */
    WINDOW_SIZE = Latency_Window_time*25/14 +100;
    printf("Receiving from %s at port %d. Deliver data to port %d\n",ServerIP,ServerPort,AppPort);
}
static void Print_help(){
    printf("Usage: rt_rcv <loss_rate_percent> <server_ip>:<server_port> <app_port> <latency_window>\n");
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

static struct timeval Half_time(struct  timeval t) {
    struct timeval returnval;
    returnval.tv_sec = t.tv_sec / 2;
    if (t.tv_sec % 2 == 1) {
        returnval.tv_usec = t.tv_usec / 2 + 500000;
    } else {
        returnval.tv_usec = t.tv_usec / 2;
    }
    return returnval;
}
