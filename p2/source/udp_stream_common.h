#ifndef CS2520_EX2_UDP_STREAM
#define CS2520_EX2_UDP_STREAM
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
#define TARGET_RATE 8 /* rate to send at, in Mbps */
#define MAX_DATA_LEN 1350
#define REPORT_SEC 5 /* print status every REPORT_SEC seconds */

#define NACK_SIZE 10
#define RECORD_SIZE 100

struct package {
    /* 0->Sender sends data to Receiver
     * 1->Sender sends ACKACK to Receiver
     * 2->Receiver sends ACK to Sender.
     * 3->Receiver sends request
     * 4->Sender sends permission
     * 5-> Sender sends decline
     * 6 -> Sender sends data to Receiver, but it is request from receiver (NACK)
     * */
    int type;

    int32_t seq;  /* Sequence number of Data Packet*/

    char data[MAX_DATA_LEN];

    int32_t ack;
    int32_t nack[NACK_SIZE];

    struct timeval Send_TS;      /*Sending time of packet.*/
    struct timeval N_Send_TS;    /*Resending time of packet(Response to NACK)*/
    struct timeval Receive_TS1;  /*First time receiver receives the packet*/
    struct timeval ACKACK_TS;    /*The time when sender gets the ACK and send ACKACK*/
    struct timeval LatencyWindow;

    int WindowSize;             /* Use for Sender initial its*/
    struct timeval Halfrtt;     /*Measure of RTT*/
};

struct stream_pkt {
    int32_t seq;
    int32_t ts_sec;
    int32_t ts_usec;
    char data[MAX_DATA_LEN];
};

#endif
