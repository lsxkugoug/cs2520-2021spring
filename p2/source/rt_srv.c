#include "net_include.h"

static int Cmp_time(struct timeval t1, struct timeval t2);


/*rt_srv <loss_rate_percent> <app_port> <client_port>*/
int rt_srv(int loss_rate_percent, int app_port, int client_port){
    sendto_dbg_init(loss_rate_percent);
    
    /*0.parameters*/
    struct sockaddr_in app_addr;                /* address to bind to app */      
    struct sockaddr_in rcv_addr;                /* address to bind to receive */        
    fd_set set;                                 /* fd set, monitor socket */
    int app,rcv;                                /* socket identifier */
    int seq = 0;                                /* latest sequence number from app */
    struct stream_pkt app_pkt;                  /* received app_pkt */
    struct  srt_pkt   rcv_pkt;   
    int WINDOW_SIZE = 0;                         /* in the request */                 
    struct timeval latencyWindow = {};          /* in the request */      
    int head = 0, tail = 1;                     /* pointers to both slide and window array header always point to null, tail always point the next slot should be encap*/
    char *client_ip = "";                       /* get ip */
    struct timeval Halfrtt = {0,30};             /* initial 1/2 RTT */
    struct timeval now;                         /* always store now time */
    int seqNum = 1;                             /* sequence number */



    /*
     *  1.bind socket with app and rcv
    */
    app = socket(AF_INET, SOCK_DGRAM, 0);
    rcv = socket(AF_INET, SOCK_DGRAM, 0);
    if (app < 0 || rcv < 0) {
        perror("app,rcv: socket");
        exit(1);
    }
    app_addr.sin_family = AF_INET;
    app_addr.sin_addr.s_addr = INADDR_ANY;
    app_addr.sin_port = htons(app_port);
    if (bind(app, (struct sockaddr *)&app_addr, sizeof(app_addr)) < 0)
    {
        perror("app: bind");
        exit(1);
    }
    if (bind(rcv, (struct sockaddr *)&rcv_addr, sizeof(rcv_addr)) < 0)
    {
        perror("rcv: bind");
        exit(1);
    }     
    printf("successfully bind the two ports app_port:%s and client_port:%s \n", app_port,client_port);
    FD_ZERO(&set);
    FD_SET(rcv, &set); /* firstly, only set rcv, after that, add app sock*/

    printf("trying to connect to receiver...\n");
    int num = select(FD_SETSIZE, &set, NULL, NULL, NULL); /*block until get the request*/
    if(num == 1){ /* send the permission */
        int getRequest = 1;
        if(FD_ISSET(rcv, &set)){
          /* receive the request until get it*/
          while(getRequest){
            int bytes = recvfrom(rcv, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr));
            if(bytes == 0||rcv_pkt.type!=3) continue;
            WINDOW_SIZE = rcv_pkt.WindowSize;
            latencyWindow = rcv_pkt.LatencyWindow;
            getRequest = 0;        
          }
          /* send the permission */
          rcv_pkt.type = 4;
          gettimeofday(&rcv_pkt.Receive_TS1, NULL);
          sendto_dbg(rcv, (char *)&rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr));
        }
    }

    /* define window and slid by malloc */
    struct srt_pkt* window = malloc(WINDOW_SIZE * sizeof(struct srt_pkt));
    struct timeval* slid = malloc(WINDOW_SIZE * sizeof(struct timeval));

            // ret = sendto(ss, (char *)&send_pkt, sizeof(send_pkt), 0,
            //     (struct sockaddr*)&send_addr, sizeof(send_addr));
    printf("successfully connect the receiver\n");

    /*
     *  2.main part
    */
    printf("trying to connect to APP...\n");
    FD_SET(app, &set);
    num = select(FD_SETSIZE, &set, NULL, NULL, NULL);
    while(num<2); //block until there are two socket in the set
    printf("successfully connect APP\n");
    while(1){
        /* 
        case1, receive app's data and send to sender
         */
        if(FD_ISSET(app, &set)){
            int bytes = recvfrom(app, &app_pkt, sizeof(app_pkt), 0, (struct sockaddr *) &app_addr, sizeof(app_addr));
            memcpy(rcv_pkt.data, app_pkt.data, sizeof(app_pkt.data));
            gettimeofday(&rcv_pkt.Send_TS, NULL); //give senTS
            rcv_pkt.type = 0;
            rcv_pkt.seq = seqNum++;
            memcpy(&window[tail%WINDOW_SIZE], &rcv_pkt, sizeof(rcv_pkt));
            sendto_dbg(rcv, (char*)&window[tail-1%WINDOW_SIZE], sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr));
            tail++;
        }

        /* 
        case2, wether should sender slide the winodw? sendTS+1/2RTT+latencyWindow<=Sender_now
        */
        gettimeofday(&now, NULL); 
        if((Cmp_time(now, slid[(head+1)%WINDOW_SIZE])||Cmp_time(now, slid[(head+1)%WINDOW_SIZE])==0)&&head<tail-1){
          head++;
        }
        
        /*
         case3, check ACK or NACK
        */
        if(FD_ISSET(rcv, &set)){
          int bytes = recvfrom(rcv, &rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr));
          /* process ACK */
          if(rcv_pkt.type==2){
            /* update Halfrtt */
            Halfrtt = rcv_pkt.Halfrtt;
            /* send ACKACK to allow rcv calculate the delta*/
            rcv_pkt.type = 1;
            gettimeofday(&rcv_pkt.ACKACK_TS, NULL); //give ACKACK_TS
            sendto_dbg(rcv, (char *)&rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr));
            /* resend loss package */
            for(int i = 0; i < NACK_SIZE; i++){
              int lossSeq = rcv_pkt.nack[i];
              int headSeq = window[head+1].seq;
              int tailSeq = window[tail-1].seq;
              /* process NACK*/
              if(lossSeq != -1 && lossSeq >= headSeq && lossSeq <= tailSeq){
                int idx = (lossSeq - headSeq + 1 + head)%WINDOW_SIZE;
                gettimeofday(&now, NULL);
                /* check time */
                struct timeval checkTm = {now.tv_sec+Halfrtt.tv_sec, now.tv_usec+Halfrtt.tv_usec};
                /* removetime > 1/2rtt + now */
                if(Cmp_time(slid[idx],checkTm)){
                  memcpy(&rcv_pkt, &window[idx],sizeof(rcv_pkt));
                  rcv_pkt.type = 6;
                  rcv_pkt.N_Send_TS = now;
                  sendto_dbg(rcv, (char *)&rcv_pkt, sizeof(rcv_pkt), 0, (struct sockaddr *) &rcv_addr, sizeof(rcv_addr));
                }
              }
            }
          }
        }
    }

    return 0;

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


