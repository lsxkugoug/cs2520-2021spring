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

#define MAX_MESS_LEN 1400
#define WINDOW_SIZE 100
#define NACK_SIZE 3
#define TMB 10000000
#define NCP_T_SEC 0
#define NCP_T_USEC 1000000
#define RCV_T_SEC 0
#define RCV_T_USEC 1000000
#define ACK_T_USEC 60

/* Only used in udp_server_hdr.c / udp_client_hdr.c to give an example of how
 * to include header data in our messages. Note that technically we should only
 * use fixed-width types in the header to ensure the header size is the same on
 * different architectures, but for assignments in this course, you don't need
 * to worry about portability across different architectures */
typedef struct dummy_uhdr {
    int cack;
    int nack[NACK_SIZE];
    int seq;
    struct timeval ts;
} uhdr;
