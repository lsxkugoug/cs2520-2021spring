#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>    
#include <fcntl.h>
#include "net_include.h"


static void Usage(int argc, char *argv[]);
static void Print_help();
static int Cmp_time(struct timeval t1, struct timeval t2);
static const struct timeval Zero_time = {0, 0};
static char* Source_file_name;
static char* Dest_file_name;
static int Port;
static char *Receiver_IP;

int main(int argc, char **argv)
{ /* udp related */
  struct sockaddr_in    send_addr;
  struct sockaddr_in    from_addr;
  socklen_t             from_len;
  struct hostent        h_ent;
  struct hostent        *p_h_ent;
  int                   host_num;
  int                   from_ip;
  int                   sock;
  fd_set                mask;
  fd_set                read_mask;
  int                   bytes;
  int                   num;
  char                  mess_buf[MAX_MESS_LEN];

  /* file related */
  FILE *fr; /* Pointer to source file, which we read */
  int nread;


  if (argc != 3)
  {
    printf("Usage: ncp <source_file_name> <dest_file_name>@<ip_address>:<port>\n");
    exit(0);
  }
  // process <dest_file_name>@<ip_address>:<port> 
  Usage(argc,argv);
  printf("Sending to %s at port %d\n", Receiver_IP, Port);
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) 
  {
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

  /* Open the source file for reading */
  if ((fr = fopen(argv[1], "r")) == NULL)
  {
    perror("fopen");
    exit(0);
  }
  printf("Opened %s for reading...\n", argv[1]);

  /* Set up mask for file descriptors we want to read from */
  int fd = open(Source_file_name,O_RDONLY|O_CREAT);
  FD_ZERO(&read_mask);
  FD_SET(sock, &read_mask);
  FD_SET(fd, &read_mask); /* stdin */

  memcpy(mess_buf, Dest_file_name, strlen(Dest_file_name) + 1);

  int filename_send = 1; // if havn't transmited the filename
  /* We'll read in the file BUF_SIZE bytes at a time, and write it 
   * BUF_SIZE bytes at a time.*/
  for (;;)
  { // file loop

    /* Read in a chunk of the file, firstly read the name, then the file*/
    if(filename_send == 0)
      nread = fread(mess_buf, 1, MAX_MESS_LEN, fr);

    /* If there is something to write, send it */
    if (nread > 0 || filename_send == 1)
    {
      
        /* (Re)set mask and timeout */
        mask = read_mask;
        /* Wait for message or timeout */
        num = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (num > 0)
        {
          if (FD_ISSET(sock, &mask))
          {
            from_len = sizeof(from_addr);
            bytes = recvfrom(sock, mess_buf, sizeof(mess_buf), 0,
                             (struct sockaddr *)&from_addr,
                             &from_len);
            mess_buf[bytes] = '\0'; /* ensure string termination for nice printing to screen */
            from_ip = from_addr.sin_addr.s_addr;

            printf("Received from (%d.%d.%d.%d): %s\n",
                   (htonl(from_ip) & 0xff000000) >> 24,
                   (htonl(from_ip) & 0x00ff0000) >> 16,
                   (htonl(from_ip) & 0x0000ff00) >> 8,
                   (htonl(from_ip) & 0x000000ff),
                   mess_buf);
          }
          else if (FD_ISSET(fd, &mask))
          {
            // bytes = read(fd, mess_buf, sizeof(mess_buf));
            // if(filename_send == 1)mess_buf[bytes] = '\0';
            sendto(sock, mess_buf, strlen(mess_buf), 0,
                   (struct sockaddr *)&send_addr, sizeof(send_addr));
            filename_send = 0;
          }
        }
      
    }

    /* fread returns a short count either at EOF or when an error occurred */
    if (nread < MAX_MESS_LEN)
    {

      /* Did we reach the EOF? */
      if (feof(fr))
      {
        printf("Finished writing.\n");
        break;
      }
      else
      {
        printf("An error occurred...\n");
        exit(0);
      }
    }
  }

  /* Cleanup */
  fclose(fr);

  return 0;
}

/* Read commandline arguments */
/* Read commandline arguments */
static void Usage(int argc, char *argv[]) {
    if (argc != 3) {
        Print_help();
    }
    Source_file_name = argv[1];
    char* delim = "@:";
    Dest_file_name = strtok(argv[2], delim);
    Receiver_IP = strtok(NULL,delim);
    Port = atoi(strtok(NULL,delim));
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
