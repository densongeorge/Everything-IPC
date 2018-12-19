
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <string.h>
#include <stdbool.h>
#include<errno.h> 
#define LISTENQ 15
#define MAXLINE 100 
#define MAX_CLIENTS 15
#include <arpa/inet.h>
int nalarms=0;
char buf[MAXLINE]={0};
int client[MAX_CLIENTS]={-1};//store client fds
bool alive[MAX_CLIENTS]={false};
int sigio[MAX_CLIENTS]={0};
int messagesent[MAX_CLIENTS]={0};
int listenfd;			//global var so that signal handlers can access them.
int connfd;
int activecons;
int firstclient=0;
int firstalrm=0;

/*Initialize the required data structures*/
void initialize()
{
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		client[i]=-1;
		alive[i]=false;
		sigio[i]=0;
		messagesent[i]=0;
	}
}

/* get the index of the client that we need to send a msg*/
int getIndex(int fd)
{
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		if(client[i]==fd)
			return i;	
	}
return -1;
}
/*connect the client to an empty index in the array*/
int getEmptyIndex()
{
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		printf("client values are %d\n", client[i]);
		if(client[i]==-1)
		{
			return i;
		}
	}
	return -1;

}

/*handling listen signals here*/
static void
sigioListenHandler (int sig, siginfo_t * si, void *ucontext)
{

  printf ("signal no:%d, for fd:%d,event code:%d,  event band:%ld\n",
	  si->si_signo, (int) si->si_fd, (int) si->si_code,
	  (long) si->si_band);
  fflush (stdout);
  if (si->si_code == POLL_IN)
    {
      struct sockaddr_in cliaddr;
      int clilen = sizeof (cliaddr);
      char ip[128];
      int n = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
	  inet_ntop (AF_INET, &(cliaddr.sin_addr), ip, 128);
      printf ("Connection accepted from %s:%d, connfd is %d\n",
	      ip,
	      ntohs (cliaddr.sin_port), n);
      if (n > 0)
      {
		connfd = n;
      fcntl (connfd, F_SETOWN, getpid ());
      int flags = fcntl (connfd, F_GETFL);	/* Get current flags */
      fcntl (connfd, F_SETFL, flags | O_ASYNC | O_NONBLOCK);
      fcntl (connfd, F_SETSIG, SIGRTMIN + 2);
      int i=getEmptyIndex();
      printf("i value is %d\n",i );
      client[i]=connfd;
      alive[i]=true;
	activecons++;
    }
     else{
	printf("Accept failed!\n");


	}
}
  if (sig == SIGIO)
    printf ("Real time signalQ overflow");
if(firstclient==0)
{
	alarm(10);
	firstclient=1;
}

}

/* SIGIO and SIGRT handler*/
static void
sigioConnHandler (int sig, siginfo_t * si, void *ucontext)
{
  printf ("signal no:%d, for fd:%d, event code:%d,  event band:%ld\n",
	  si->si_signo, (int) si->si_fd, (int) si->si_code,
	  (long) si->si_band);
  fflush (stdout);
  if (si->si_code == POLL_OUT)
    {
      //output possible
      printf ("POLL_OUT event occured\n");
    }
  if (si->si_code == POLL_IN)
    {				//input available
      printf ("POLL_IN event occured\n");

      int n = read (si->si_fd, buf, MAXLINE);
      int index=getIndex(si->si_fd);
      if(strncmp(buf,"ALIVE",5)==0)
      {
      	
      	alive[index]=true;
      	sigio[index]=1;
      	printf("ALIVE detected\n" );
      }
      else{

      	if (n == 0)
		{
		  close (si->si_fd);
		  printf ("EOF read. Socket %d closed\n", si->si_fd);
		}
		else if (n > 0)
			{
			  buf[n] = '\0';
			  printf ("Data from connfd %d:len %d: %s \n", si->si_fd,n, buf);
			for(int i=0;i<MAX_CLIENTS;i++)
			{
			if(client[i]!=si->si_fd && alive[i]==true)
			{
			  if (write (client[i], buf, n) < 0)
			    {
			      perror ("write");
			    }
			}
			}
			}
	else
	{
		if(n<0)
			if(errno=EINTR)
				printf("read call interrupted\n");
			else
			{
				perror("readError");
				exit(-1);
			}
	}
}
}
  if (si->si_code == POLL_ERR)
    {
      int err;
      int errlen = sizeof (int);
      getsockopt (si->si_fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
      if (err > 0)
	printf ("error on socket %d : %s", si->si_fd, strerror (err));
    }
  fflush (stdout);
	
}

/*SIGALRM handler*/
static void
sigAlaramHandler (int sig, siginfo_t * si, void *ucontext)
{
  printf ("signal no:%d, for fd:%d, event code:%d,  event band:%ld\n",si->si_signo, (int) si->si_fd, (int) si->si_code,
	  (long) si->si_band);
  fflush (stdout);
	nalarms++;
	if((nalarms%2)==0)
	{
		if(activecons>0)
		{
		//send heartbeat messages to everyone
		for(int i=0;i<MAX_CLIENTS;i++)
	{
		if(alive[i]==true)
		{
	  if (write (client[i], "Please respond with ALIVE within 10 seconds\n", strlen( "Please respond with ALIVE within 10 seconds\n")) < 0)
	    {
	      perror ("write");
	    }
		else
		{
			sigio[i]=0;
			messagesent[i]=1;
		}
	}
	}
		}
	}
	else
	{
		if(firstalrm==1)
		{
			if(activecons>0)
		{
			for(int i=0;i<MAX_CLIENTS;i++)
			{
				if(alive[i]==true &&sigio[i]==1)
				{
					printf("client is alive with %d\n",client[i]);

				}
				else
				{
					if(client[i]!=-1&&messagesent[i]==1)
					{
					printf("client with %d exited\n",client[i]);
					close(client[i]);
					alive[i]=false;
					client[i]=-1;
					messagesent[i]=0;
					activecons--;
		 			sigio[i]=0;
		 			}
		 			continue;
				}
			}
		}
}

}
firstalrm=1;
alarm(10);
}

int 
main (int argc, char **argv)
{
	if(argc!=2)
	{
		printf("Add port number as argument\n");
		exit(EXIT_FAILURE);
	}
	//initialize the arrays
	initialize();
  char buf[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in cliaddr, servaddr;
  struct sigaction sa, sa1,sigalrm;
  memset (&sa, '\0', sizeof (sa));
  memset (&sa1, '\0', sizeof (sa1));
 memset (&sigalrm, '\0', sizeof (sigalrm));
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = &sigioListenHandler;	//for accepting new conn
  sigaction (SIGIO, &sa, NULL);
  sigaction (SIGRTMIN + 1, &sa, NULL);

  sigemptyset (&sa1.sa_mask);
  sa1.sa_flags = SA_SIGINFO;
  sa1.sa_sigaction = &sigioConnHandler;	//for reading data
  sigaction (SIGRTMIN + 2, &sa1, NULL);


//for handling the alarm signal
sigset_t set;
sigemptyset(&set);
sigaddset(&set, SIGIO);
sigaddset(&set, SIGRTMIN + 1);
sigaddset(&set,SIGRTMIN + 2);
sigemptyset (&sigalrm.sa_mask);
sigalrm.sa_mask=set;
  sigalrm.sa_flags = SA_SIGINFO;
  sigalrm.sa_sigaction = &sigAlaramHandler;	
  sigaction (SIGALRM, &sigalrm, NULL);

  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (atoi (argv[1]));
  bind (listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr));
  listen (listenfd, LISTENQ);

  fcntl (listenfd, F_SETOWN, getpid ());
  int flags = fcntl (listenfd, F_GETFL);	/* Get current flags */
  fcntl (listenfd, F_SETFL, flags | O_ASYNC | O_NONBLOCK);	//set signal driven IO
  fcntl (listenfd, F_SETSIG, SIGRTMIN + 1);	//replace SIGIO with realtime signal
  
	
  for (;;)
    {
      pause ();
    }
return 0;
}
