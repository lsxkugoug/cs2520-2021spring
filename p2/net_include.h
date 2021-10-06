#include <stdio.h>
#include <stdlib.h>
#include "sendto_dbg.h"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <errno.h>

#define MAX_MESS_LEN 4000
#define WINDOW_SIZE 200
#define NACK_SIZE 3
#define RECORD_SIZE 50
#define TMB 10000000

typedef struct dummy_uhdr {
    /* 0-> Sender sends data to Receiver
     * 1->Sender sends ACKACK
     * 2->Sender Receive ACK from Receiver
     * 3->Receiver sends request
     * 4->Sender sends permission
     * 5-> Sender sends decline
     * */
    int     type;

    int32_t seq;  /* Sequence number of Data Packet*/

    int32_t ack;
    int32_t nack[NACK_SIZE];
    int32_t ackack;

    struct timeval DeliverTime; /*The delivery time of data*/
    struct timeval Send_TS;     /*Send time of packet. It could change when we resend the Packet*/
    struct timeval Receive_TS1; /*First time receiver receives the packet*/
    struct timeval ACKACK_TS;   /*The time when sender gets the ACK and send ACKACK*/

    struct timeval base_delta;  /*Measure of ½ RTT+ clock _diff*/
    struct timeval rtt;         /*Measure of RTT*/

} uhdr;
