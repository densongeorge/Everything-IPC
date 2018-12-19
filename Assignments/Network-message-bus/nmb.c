#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include<errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#define PORT 1111		//TCP port specified here.
#define BUFF 1024
#define checkError(eno,msg)             \
 ({                                      \
         if(eno<0)                       \
         {                               \
                 perror(msg);            \
              exit(EXIT_FAILURE);     \
         }                               \
 })


struct payload
{
  int option;

  char data[1024];

};
struct msg
{
  unsigned long mtype;
  struct payload pload;
};


uint32_t
extractip (unsigned long mtype)
{
  unsigned char *ip = (unsigned char *) &mtype;
  ip = ip + 2;
  uint32_t ipaddr = 0;
  memcpy ((unsigned char *) &ipaddr, ip, 4);
  return ipaddr;
}

unsigned short
extractport (unsigned long mtype)
{
  unsigned char *port = (unsigned char *) &mtype;
  port = port + 6;
  unsigned short portaddr = 0;
  memcpy ((char *) &portaddr, port, 2);
  return portaddr;
}

unsigned long
convert (char destip[], unsigned short destport)
{
  uint32_t ip = 0;
  unsigned short port = htons (destport);
  unsigned long mtype = 0;
  int rc = inet_pton (AF_INET, destip, &ip);
  checkError (rc, "Errorconvertingip");
  unsigned char *ipport = { 0 };
  ipport = (unsigned char *) &mtype;
  memset (ipport, '\0', 8);
  unsigned char *ipp = ipport;
  ipp = ipp + 2;
  memcpy (ipp, (unsigned char *) &ip, 4);
  ipp = ipp + 4;
  memcpy (ipp, (unsigned char *) &port, 2);
  return mtype;
}

void
msgsnd_nmb (char ip[], unsigned short port, char data[])
{
  struct sockaddr_in address;
  struct msg p;
  p.mtype = convert (ip, port);
  p.pload.option = 1;		// used to identify sending event at the server
  strcpy (p.pload.data, data);
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  sock = socket (AF_INET, SOCK_STREAM, 0);
  checkError (sock, "TCPSocketCreateError");
  memset (&serv_addr, '0', sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (PORT);

  if (inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
      printf ("\nInvalid address/ Address not supported \n");
    }

  int rc = connect (sock, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
  checkError (rc, "TCPconnecterror");
  send (sock, &p, sizeof (p), 0);	//send to local tcp server

}



struct msg
msgrcv_nmb (char ip[], unsigned short port)
{
  struct msg p;
  if(port < 1024)
	{
		printf("Please enter a port greater than 1023\n");
		exit(EXIT_FAILURE);
	}
  memset (&p, '0', sizeof (p));
  p.mtype = convert (ip, port);
  struct sockaddr_in address;
  p.pload.option = 2;		// used to identify recieve at the server
  int sock = 0, valread;
  struct sockaddr_in serv_addr, client_addr;
  sock = socket (AF_INET, SOCK_STREAM, 0);
  checkError (sock, "TCPrcvsockCreationErr");
  memset (&serv_addr, '0', sizeof (serv_addr));
  memset (&client_addr, '0', sizeof (client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = INADDR_ANY;
  client_addr.sin_port = htons (port);
  int true = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &true, sizeof (int));
//bind to the specified port now
  int ret =
    bind (sock, (struct sockaddr *) &client_addr, sizeof (client_addr));
  checkError (ret, "CLIENTSOCKBINDERROR");
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (PORT);

  if (inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
      printf ("\nInvalid address/ Address not supported \n");
    }

  int rc = connect (sock, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
  checkError (rc, "CLIENTSOCKConnectERROR");
  send (sock, &p, sizeof (p), 0);
  printf ("Please wait fetching your message!!\n");
//perhaps put a sleep here.
  sleep (2);
  int n = recv (sock, &p, sizeof (p), 0);

  close (sock);
  /* if (errno == EWOULDBLOCK)
     {
     printf ("NO MESSAGE FOR SPECIFIED PROCESS");
     exit (EXIT_FAILURE);

     }
   */
  return p;

}
