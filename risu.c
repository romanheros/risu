/* Random Instruction Sequences for Userspace */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>


int apprentice_connect(const char *hostname, uint16_t port)
{
   /* We are the client end of the TCP connection */
   int sock;
   struct sockaddr_in sa;
   sock = socket(PF_INET, SOCK_STREAM, 0);
   if (sock < 0)
   {
      perror("socket");
      exit(1);
   }
   struct hostent *hostinfo;
   sa.sin_family = AF_INET;
   sa.sin_port = htons(port);
   hostinfo = gethostbyname(hostname);
   if (!hostinfo)
   {
      fprintf(stderr, "Unknown host %s\n", hostname);
      exit(1);
   }
   sa.sin_addr = *(struct in_addr*)hostinfo->h_addr;
   if (connect(sock, (struct sockaddr*)&sa, sizeof(sa)) < 0)
   {
      perror("connect");
      exit(1);
   }
   return sock;
}

int master_connect(uint16_t port)
{
   int sock;
   struct sockaddr_in sa;
   sock = socket(PF_INET, SOCK_STREAM, 0);
   if (sock < 0)
   {
      perror("socket");
      exit(1);
   }
   int sora = 1;
   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sora, sizeof(sora)) != 0)
   {
      perror("setsockopt(SO_REUSEADDR)");
      exit(1);
   }

   sa.sin_family = AF_INET;
   sa.sin_port = htons(port);
   sa.sin_addr.s_addr = htonl(INADDR_ANY);
   if (bind(sock, (struct sockaddr*)&sa, sizeof(sa)) < 0)
   {
      perror("bind");
      exit(1);
   }
   if (listen(sock, 1) < 0)
   {
      perror("listen");
      exit(1);
   }
   /* Just block until we get a connection */
   fprintf(stderr, "master: waiting for connection on port %d...\n", port);
   struct sockaddr_in csa;
   size_t csasz;
   int nsock = accept(sock, (struct sockaddr*)&csa, &csasz);
   if (nsock < 0)
   {
      perror("accept");
      exit(1);
   }
   /* We're done with the server socket now */
   close(sock);
   return nsock;
}

/* The communication protocol is very simple.
 * The apprentice sends a packet like this:
 *  4 bytes: faulting insn
 *  n bytes: register dump
 * The master replies with a single byte response:
 *  0: ok, continue  [registers matched]
 *  1: exit with status 0 [end of test]
 *  2: exit with status 1 [mismatch, fail]
 */

int send_data_pkt(int sock, void *pkt, int pktlen)
{
   unsigned char resp;
   char *p = pkt;
   while (pktlen)
   {
      int i = write(sock, p, pktlen);
      if (i <= 0)
      {
         perror("write failed");
         exit(1);
      }
      pktlen -= i;
      p += i;
   }
   if (read(sock, &resp, 1) != 1)
   {
      perror("read failed");
      exit(1);
   }
   return resp;
}

void recv_data_pkt(int sock, void *pkt, int pktlen)
{
   /* We always read a fixed length packet */ 
   char *p = pkt;
   while (pktlen)
   {
      int i = read(sock, p, pktlen);
      if (i <= 0)
      {
         perror("read failed");
         exit(1);
      }
      pktlen -= i;
      p += i;
   }
}

void send_response_byte(int sock, int resp)
{
   unsigned char r = resp;
   if (write(sock, &r, 1) != 1)
   {
      perror("write failed");
      exit(1);
   }   
}

int master(int sock)
{
   printf("master waiting for data\n");
   unsigned char cmd;
   recv_data_pkt(sock, &cmd, 1);
   printf("Got cmd %c, sending resp 0\n", cmd);
   send_response_byte(sock, 0);
   printf("Exiting.\n");
   return 0;
}

int apprentice(int sock)
{
   printf("requesting stop\n");
   int resp = send_data_pkt(sock, "S", 1);
   printf("got response %d\n", resp);
   return resp;
}


int ismaster;

int main(int argc, char **argv)
{
   // some handy defaults to make testing easier
   uint16_t port = 9191;
   char *hostname = "localhost";
   int sock;
   

   // TODO clean this up later
   
   for (;;)
   {
      static struct option longopts[] = 
         {
            { "master", no_argument, &ismaster, 1 },
            { "host", required_argument, 0, 'h' },
            { "port", required_argument, 0, 'p' },
            { 0,0,0,0 }
         };
      int optidx = 0;
      int c = getopt_long(argc, argv, "h:p:", longopts, &optidx);
      if (c == -1)
      {
         break;
      }
      
      switch (c)
      {
         case 0:
         {
            /* flag set by getopt_long, do nothing */
            break;
         }
         case 'h':
         {
            hostname = optarg;
            break;
         }
         case 'p':
         {
            // FIXME err handling
            port = strtol(optarg, 0, 10);
            break;
         }
         case '?':
         {
            /* error message printed by getopt_long */
            exit(1);
         }
         default:
            abort();
      }
   }
   if (ismaster)
   {
      printf("master port %d\n", port);
      sock = master_connect(port);
      return master(sock);
   }
   else
   {
      printf("apprentice host %s port %d\n", hostname, port);
      sock = apprentice_connect(hostname, port);
      return apprentice(sock);
   }
}

   
