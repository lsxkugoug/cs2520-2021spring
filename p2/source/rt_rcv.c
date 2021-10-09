#include "net_include.h"
#include "udp_stream_common.h"
static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static struct timeval Half_time(struct  timeval t);
static const struct timeval Zero_time = {0, 0};
static struct timeval Base_Delta ;
static struct timeval Half_RTT = {10,0};
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
    int Getpermission=0

    struct timeval now;
    struct timeval timer_initial;
    struct timeval timer_finish;
    struct timeval diff_time;
    struct timeval deliver_time;
    static struct timeval base_delta ;
    static struct timeval half_rtt ;
    struct timeval temp1;
    struct timeval temp2;

    /* Packet to UPD*/
    struct stream_pkt send_pkt;

    /* Packet echo back to Sender*/
    struct str_pkt echo_pkt;
    /* Packet from sender */
    struct str_pkt sender_pkt;

    struct str_pkt temp_pkt;

    /* Latest Record of Delta and RTT */
    int32_t delta_sec[RECORD_SIZE]={0};
    int32_t delta_usec[RECORD_SIZE] = {0};
    int32_t delta_i = 0;
    int32_t rtt_sec[RECORD_SIZE]={0};
    int32_t rtt_usec[RECORD_SIZE] = {0};
    int32_t rtt_i = 0;

    int32_t C_ack = 0;

    /*--------------Main Part of Program--------------------*/
    Usage(argc,argv);
    /* Initialization the Latency Window */
    struct srt_pkt *window = malloc(WINDOW_SIZE * sizeof(struct srt_pkt));
    int buffersize = 0;
    int buffer[WINDOW_SIZE] = {0};

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

    /* Set up mask for sock */
    FD_ZERO(&read_mask);
    FD_SET(sock, &read_mask);

    /*--------------Connection Part--------------------*/
    /* Keep Sending request every 100000000 ns(0.1s) until we get reply from sender  */
    gettimeofday(&timer_initial,NULL);
    while(!Getpermission){
        mask = read_mask;
        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if(num>0){
            if (FD_ISSET(sock, &mask)) {
                Sender_len = sizeof(Sender_addr);
                bytes = recvfrom(sock, &sender_pkt, sizeof(sender_pkt), 0,(struct sockaddr *) &Sender_addr,&Sender_len);
                /* Sender sends the permission.*/
                /* Basically, We could calculate initial RTT and Base Delta*/
                if(sender_pkt.type == 4){
                    Getpermission = 1;
                    gettimeofday(&now, NULL);
                    timersub(&now, &sender_pkt.Receive_TS1, &Base_Delta);
                    timersub(&now,&sender_pkt.Send_TS,&diff_time);
                    Half_RTT = Half_time(diff_time);
                    rtt_sec[rtt_i] = Half_RTT.tv_sec;
                    rtt_usec[rtt_i] = Half_RTT.tv_usec;
                    delta_sec[delta_i] = Base_Delta.tv_sec;
                    delta_usec[delta_i] = Base_Delta.tv_usec;
                    rtt_i++;
                    delta_i++;
                    puts("Successfully connected with Sender!");
                }
                /* Since the sender only sends one permission, if the permission get lost, We could still
                 * get permission when sender sends data packets sendTS get now
                 * */
                if(sender_pkt.type == 0){
                    Getpermission = 1;
                    gettimeofday(&sender_pkt.Receive_TS1, NULL);
                    timersub(&sender_pkt.Receive_TS1,&sender_pkt.Send_TS,&Base_Delta);
                    delta_sec[delta_i] = Base_Delta.tv_sec;
                    delta_usec[delta_i] = Base_Delta.tv_usec;
                    delta_i++;
                    puts("Successfully connected with Sender!");
                    /* Put received packet into window*/
                    memcpy(&window[sender_pkt.seq%WINDOW_SIZE],&sender_pkt,sizeof(sender_pkt));
                    buffersize++;
                    buffer[sender_pkt.seq%WINDOW_SIZE] = 1;
                    /* Fill in the echo message */
                    echo_pkt.type = 2;
                    timeradd(&Zero_time,&sender_pkt.Send_TS,&echo_pkt.Send_TS);
                    gettimeofday(&echo_pkt.Receive_TS1, NULL);
                    echo_pkt.seq = sender_pkt.seq;
                    echo_pkt.ack = sender_pkt.seq;
                    for(int i = 0;i<NACK_SIZE;i++){echo_pkt.nack[i] = -1;}
                    echo_pkt.Halfrtt.tv_sec =Half_RTT.tv_sec;
                    echo_pkt.Halfrtt.tv_usec = Half_RTT.tv_usec;
                    sendto_dbg(sock, (char *)&echo_pkt, sizeof(echo_pkt), 0,
                               (struct sockaddr *) &send_addr, sizeof(send_addr));
                }
                /* Sender sends decline */
                if(sender_pkt.type == 5){
                    puts(" Server is being occupied by another Client!");
                    exit(0);
                }
            }
        }
        gettimeofday(&timer_finish,NULL);
        timersub(&timer_finish,&timer_initial,&diff_time);
        if(diff_time.tv_sec>0 || diff_time.tv_usec >= 100000000){
            gettimeofday(&hdr->Send_TS,NULL);
            hdr->type = 3;
            hdr->WindowSize= WINDOW_SIZE;
            hdr->LatencyWindow = Latency_Window;
            sendto_dbg(sock, mess_buf, sizeof(mess_buf), 0,
                   (struct sockaddr *) &send_addr, sizeof(send_addr));
            gettimeofday(&timer_initial, NULL);
        }
    }

    /*  Create instance of Local APP address */
    Local_p_h_ent = gethostbyname(localIP);
    if(Local_p_h_ent == NULL){puts("Local ip error!");exit(1);}
    memcpy(&Local_h_ent, Local_p_h_ent, sizeof(Local_h_ent));
    memcpy(&Local_host_num, h_ent.h_addr_list[0], sizeof(host_num));
    Localapp_addr.sin_family = AF_INET;
    Localapp_addr.sin_addr.s_addr = Local_host_num;
    Localapp_addr.sin_port = htons(AppPort);

    for(;;){
        /*-------Look up the window,and deliver the packet on Delivery Time.-------*/
        gettimeofday(&now,NULL);
        /* Stop case 1: Stop deliver when we do not have anything in window */
        for(int i = C_ack+1; i<C_ack+1+WINDOW_SIZE || buffersize==0 ;i++){
            /*If we miss some packets, we will still check the next packet, for example , 12 456*/
            if(buffer[i%WINDOW_SIZE]==0){continue;}
            temp_pkt = window[i%WINDOW_SIZE];
            /* Now equal to or above sendTS+base_delta+latency window We should deliver the data */
            timeradd(&Base_Delta, temp_pkt.Send_TS, &deliver_time);
            timeradd(&deliver_time,&Latency_Window,&deliver_time);
            int cmp = Cmp_time(now,deliver_time);
            if(cmp ==0 || cmp==1 ){
                send_pkt.seq = temp_pkt.seq;
                send_pkt.ts_sec = now.tv_sec;
                send_pkt.ts_usec = now.tv_usec;
                memcpy(&send_pkt.data,&temp_pkt.data,sizeof(temp_pkt.data));
                sendto(sock, (char *)&send_pkt, sizeof(send_pkt), 0,
                       (struct sockaddr *) &Localapp_addr, sizeof(Localapp_addr));
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
        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
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
                    /* Check whether the sendTS match the requirement: sendTS+ Base_Delta+ latencyWindow >= now */
                    timeradd(&sender_pkt.Send_TS,&Base_Delta,&temp1);
                    timeradd(&temp1,&Latency_Window,&temp1);
                    gettimeofday(&now,NULL);
                    timersub(&temp1,&now,&temp2);
                    int k = Cmp_time(temp2,Zero_time);
                    if(k==1 || k==0 ) {
                        int32_t t1,t2,t3;
                        /* Update the baseDelta*/
                        gettimeofday(&now, NULL);
                        gettimeofday(&sender_pkt.Receive_TS1, NULL);
                        if(sender_pkt.type==0){
                            timersub(&now,&sender_pkt.Send_TS,&base_delta);
                        }
                        else{
                            timersub(&now,&sender_pkt.N_Send_TS,&base_delta);
                        }
                        if(delta_i<RECORD_SIZE) {
                            delta_sec[delta_i] = base_delta.tv_sec;
                            delta_usec[delta_i] = base_delta.tv_usec;
                            t1 = (Base_Delta.tv_sec*delta_i+base_delta.tv_sec)/(delta_i+1);
                            t2 = (Base_Delta.tv_sec*delta_i+base_delta.tv_sec) - t1*(delta_i+1);
                            t3 = (t2*1000000000 + Base_Delta.tv_usec*delta_i+base_delta.tv_usec) /(delta_i+1);
                            Base_Delta.tv_sec = t1;
                            Base_Delta.tv_usec = t3;
                            delta_i++;
                        }
                        else{
                            t1 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE])/RECORD_SIZE;
                            t2 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE]) - t1*RECORD_SIZE;
                            t3 = (t2*1000000000 + base_delta.tv_usec - delta_usec[delta_i % RECORD_SIZE])/RECORD_SIZE;
                            Base_Delta.tv_sec += t1;
                            Base_Delta.tv_usec+= t3;
                            delta_sec[delta_i % RECORD_SIZE] = base_delta.tv_sec;
                            delta_usec[delta_i % RECORD_SIZE] = base_delta.tv_usec;
                            delta_i++;
                        }

                        /* Put received packet into window*/
                        memcpy(&window[sender_pkt.seq%WINDOW_SIZE],&sender_pkt,sizeof(sender_pkt));
                        buffersize++;
                        buffer[sender_pkt.seq%WINDOW_SIZE] = 1;

                        /* Fill in the echo message */
                        echo_pkt.type = 2;
                        timeradd(&Zero_time,&sender_pkt.Send_TS,&echo_pkt.Send_TS);
                        gettimeofday(&echo_pkt.Receive_TS1, NULL);
                        echo_pkt.seq = sender_pkt.seq;
                        echo_pkt.ack = sender_pkt.seq;
                        for(int i = 0;i<NACK_SIZE;i++){echo_pkt.nack[i] = -1;}
                        /* Find at most Nacksize Nacks for example 3*/
                        for (int i = C_ack + 1, j = 0; i < sender_pkt.seq && j < NACK_SIZE; i++) {
                            if (buffer[i] == 0) {
                                echo_pkt.nack[j]=i;
                                j++;
                            }
                        }
                        echo_pkt.Halfrtt.tv_sec = Half_RTT.tv_sec;
                        echo_pkt.Halfrtt.tv_usec = Half_RTT.tv_usec;

                        sendto_dbg(sock, (char *)&echo_pkt, sizeof(echo_pkt), 0,
                                   (struct sockaddr *) &send_addr, sizeof(send_addr));
                    }
                }
                /* Case 2: Sender sends ACKACK to Receiver (Update the base_delta and halfRTT) */
                if(sender_pkt.type == 1){
                    int32_t t1,t2,t3;
                    /* The receiver gets ACK ACK and update Base Delta and Half_RTT */
                    gettimeofday(&now,NULL);
                    /* Base Delta */
                    timersub(&now,&sender_pkt.ACKACK_TS,&base_delta);
                    if(delta_i<RECORD_SIZE) {
                        delta_sec[delta_i] = base_delta.tv_sec;
                        delta_usec[delta_i] = base_delta.tv_usec;
                        t1 = (Base_Delta.tv_sec*delta_i+base_delta.tv_sec)/(delta_i+1);
                        t2 = (Base_Delta.tv_sec*delta_i+base_delta.tv_sec) - t1*(delta_i+1);
                        t3 = (t2*1000000000 + Base_Delta.tv_usec*delta_i+base_delta.tv_usec) /(delta_i+1);
                        Base_Delta.tv_sec = t1;
                        Base_Delta.tv_usec = t3;
                        delta_i++;
                    }
                    else{
                        t1 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE])/RECORD_SIZE;
                        t2 = (base_delta.tv_sec - delta_sec[delta_i % RECORD_SIZE]) - t1*RECORD_SIZE;
                        t3 = (t2*1000000000 + base_delta.tv_usec - delta_usec[delta_i % RECORD_SIZE])/RECORD_SIZE;
                        Base_Delta.tv_sec += t1;
                        Base_Delta.tv_usec+= t3;
                        delta_sec[delta_i % RECORD_SIZE] = base_delta.tv_sec;
                        delta_usec[delta_i % RECORD_SIZE] = base_delta.tv_usec;
                        delta_i++;
                    }

                    /* Half of RTT */
                    timersub(&now,&sender_pkt.Receive_TS1,&temp1);// RTT
                    if(sender_pkt.N_Send_TS.tv_usec == NULL) {
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
                            t3 = (t2 * 1000000000 + Half_RTT.tv_usec * rtt_i + half_rtt.tv_usec) / (rtt_i + 1);
                            Half_RTT.tv_sec = t1;
                            Half_RTT.tv_usec = t3;
                            rtt_i++;
                        }
                    }else {
                        t1 = (half_rtt.tv_sec - rtt_sec[rtt_i % RECORD_SIZE]) / RECORD_SIZE;
                        t2 = (half_rtt.tv_sec - rtt_sec[rtt_i % RECORD_SIZE]) - t1 * RECORD_SIZE;
                        t3 = (t2 * 1000000000 + half_rtt.tv_usec - rtt_usec[rtt_i % RECORD_SIZE]) / RECORD_SIZE;
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
    }
    return 0;
}


static void Usage(int argc, char *argv[]){
    if (argc != 5){Print_help();}
    Loss_rate = atoi(argv[1]);
    sendto_dbg_init(Loss_rate);
    char* delim = ":";
    ServerIP = strtok(argv[2],delim);
    Server_Port = atoi(strtok(NULL,delim));
    AppPort = atoi(argv[3]);
    Latency_Window_time = atoi(argv[4]);
    Latency_Window.tv_sec = Latency_Window_time/1000;
    Latency_Window.tv_usec = (Latency_Window_time- Latency_Window.tv_sec*1000)*1000000;
    /*T(ns) * 2.5M bytes /s / 1400 bytes = T *2.5*10^3/ 1400 = T*25/14 */
    WINDOW_SIZE = Latency_Window_time*25/14 +1;
    printf("Receiving from %s at port %d. Deliver data to port %d",ServerIP,ServerPort,AppPort);
}
static void Print_help(){
    printf("Usage: rt_rcv <loss_rate_percent> <server_ip>:<server_port> <app_port> <latency_window>\n");
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

static struct timeval Half_time(struct  timeval t){
    struct timeval returnval;
    returnval.tv_sec = t.tv_sec/2;
    if(t.tv_sec%2 ==1 ){
        returnval.tv_usec = t.tv_usec/2+ 500000000;
    }else{
        returnval.tv_usec = t.tv_usec/2;
    }
    return returnval;
}