#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include<signal.h>
#include<errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <string.h>
#include<errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define key 2000
#include "nmb.c"
#define PORT 1111

#define UDPPORT 1112

#define checkError(eno,msg)               \
  ({                                      \
          if(eno<0)                       \
          {                               \
                  perror(msg);            \
                  exit(EXIT_FAILURE);     \
         }                                \
 })

void
handle_signal (int sig)
{
  int pid = 0;
  while ((pid = waitpid (-1, NULL, WNOHANG)) > 0);

}

int
check_endianness ()
{
  int flag = 0;
  unsigned int i = 1;
  char *c = (char *) &i;
  if (*c)
    flag = 1;			//little endian
  //otherwise big endian


}

//this is where the udp server is defined
void
udplistener (char *ip, int queueId)
{

  struct msg r;
  struct sockaddr_in udpserveraddress, clientaddress;
  udpserveraddress.sin_family = AF_INET;
  udpserveraddress.sin_addr.s_addr = htonl (INADDR_ANY);
  udpserveraddress.sin_port = htons (UDPPORT);
  int udp_server_fd = socket (AF_INET, SOCK_DGRAM, 0);
  checkError (udp_server_fd, "UDPServSocketError");
  int rc = bind (udp_server_fd, (struct sockaddr *) &udpserveraddress,
		 sizeof (udpserveraddress));
  checkError (rc, "UDPBindSocketError");


  uint32_t ipinnbo = 0;

//convert ip into binary representation
  if (inet_pton (AF_INET, ip, &ipinnbo) <= 0)
    {
      printf ("\nInvalid address/ Address not supported \n");
    }

  while (1)
    {
      int len = sizeof (clientaddress);
      int n = 0;
      char i[16] = { 0 };
      uint32_t cip = 0;
      int flag = 1;		//little endian assumption
      n = recvfrom (udp_server_fd, (struct msg *) &r, sizeof (r),
		    0, (struct sockaddr *) &clientaddress, &len);

      cip = extractip (r.mtype);
      flag = check_endianness ();
      if (flag)
	r.mtype = __builtin_bswap64 (r.mtype);

//add this data into the message queue if the ip matches
      if (ipinnbo == cip)
	{

	  i[15] = '\0';
	  int ret = msgsnd (queueId, &r, sizeof (r.pload), 0);

	  checkError (ret, "ERRORinmsgQ");
	}

    }
}

int
main (int argc, char const *argv[])
{
  struct msg p, r;
  if (argc < 2)
    {
      printf
	("NOt enough arguments! Enter ip from ifconfig in dotted decimal format!");
      exit (EXIT_FAILURE);
    }

  char ip[16] = { 0 };
  strcpy (ip, argv[1]);

//create the message queue
  int queueId = msgget (key, IPC_CREAT | 0666);
  checkError (queueId, "ERRORincreatingmsgQ");

//zero out the structures
  memset (&p, '0', sizeof (p));
  int server_fd, udp_client_fd, new_socket, count;
  pid_t childserver;
  struct sockaddr_in address, acceptedclientadd, clientaddress,
    udpserveraddress;
  int opt = 1;
  int addrlen = sizeof (acceptedclientadd);
  char buffer[1024] = { 0 };
  //register signal handler for sigchild
  signal (SIGCHLD, handle_signal);
  int rc = 0;
  // Creating socket file descriptor 
  server_fd = socket (AF_INET, SOCK_STREAM, 0);
  checkError (server_fd, "TCPSOCKETfailed");
  //Create the UDP socket
  udp_client_fd = socket (AF_INET, SOCK_DGRAM, 0);
  fcntl (udp_client_fd, F_SETFL, O_NONBLOCK);
  checkError (udp_client_fd, "UDPSOCKETfailed");
  int cid = fork ();
  checkError (cid, "fork1failed");
  if (cid == 0)
    {
      udplistener (ip, queueId);
    }
  else
    {
      memset (&clientaddress, '0', sizeof (clientaddress));
      // Forcefully attaching socket to the port  
      rc = setsockopt (server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		       &opt, sizeof (opt));
      checkError (rc, "SetSockTCPFailed");
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port = htons (PORT);
      // Forcefully attaching socket to the port  
      rc =
	setsockopt (udp_client_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		    &opt, sizeof (opt));
      checkError (rc, "SetSockUDPFailed");
      // Forcefully attaching socket to the port  
      rc = bind (server_fd, (struct sockaddr *) &address, sizeof (address));
      checkError (rc, "BindSockTCPFailed");
      clientaddress.sin_family = AF_INET;
      clientaddress.sin_port = htons (1112);
      rc = listen (server_fd, 3);
      checkError (rc, "ListenTCP");

      for (;;)
	{
	  if ((new_socket =
	       accept (server_fd, (struct sockaddr *) &acceptedclientadd,
		       (socklen_t *) & addrlen)) < 0)
	    {
	      if (errno == EINTR || errno == EAGAIN)
		continue;
	      else
		{
		  checkError (new_socket, "AcceptTCP");
		}
	    }
	  if ((childserver = fork ()) == 0)
	    {
	      close (server_fd);
	      count = read (new_socket, &p, sizeof (p));
	      clientaddress.sin_addr.s_addr = extractip (p.mtype);
	      if (p.pload.option == 1)	// send event
		{
		  rc =
		    sendto (udp_client_fd, (const struct msg *) &p,
			    sizeof (p), MSG_CONFIRM,
			    (const struct sockaddr *) &clientaddress,
			    sizeof (clientaddress));
		  checkError (rc, "UDPsenderror");
		  exit (0);
		}

	      if (p.pload.option == 2)	// recieve event.
		{
		  r.pload.option = 2;
		  int flag = 1;
		  flag = check_endianness ();
		  if (flag)
		    r.mtype = __builtin_bswap64 (p.mtype);	//this is required as otherwise the long number byte order will be swapped and interpreted as a different number then was stored in memory
		  int ret =
		    msgrcv (queueId, (void *) &r, sizeof (r), r.mtype,
			    IPC_NOWAIT);
		  if (ret < 0)
		    strcpy (r.pload.data, "No Message recieved for this process!");
		  send (new_socket, &r, sizeof (r), 0);
		  exit (0);
		}
	    }
	}
    }
  return 0;
}
