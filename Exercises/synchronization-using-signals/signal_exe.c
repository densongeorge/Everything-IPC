#include<unistd.h>
#include<stdio.h>
#include <sys/wait.h>
#include<signal.h>
#include<sys/types.h>
#include<errno.h>
#include<stdlib.h>
#include<time.h>
#define ARRAY_SIZE 1000 

/* printArray Function
 *input parameters : input array,index
 *returns: NONE
 *
 */
void printArray(int* input,int index)
{	
//	  fflush(stdout);
          printf("%d(%d) \t",getpid(),input[index]);
//	  fflush(stdout);

}

//SIGUSR1 handler function
static void sighandler1(int sig)
{
	//Do nothing
}

/* cleanP Function
 *input parameters : NONE
 *returns: 
 *-1: error in cleanup
 *0: successful cleanup
 */

int  cleanP()
{
  if (kill(0, SIGINT) < 0)
	  return -1;
 return 0;
	 
}

int main(int argc, char *argv[]) 
{

    if(argc<=1) 
    {
        printf("Please provide the  number of processes as argument\n");
        exit(EXIT_FAILURE);
     }

     int n = atoi(argv[1]);//n holds number of processes

     //pid of the main parent process
     int pid = getpid();

     //declare the input array
     int input[ARRAY_SIZE];
     int cpid = 0;

     //calculate the number of iterations over the input array
     int numitrs=ARRAY_SIZE/n;

     //unique sequential index for all created processes starting from 0
     int process_no=0;

     //if indices are left increment the number of iterations
     if((ARRAY_SIZE)%n > 0)
	     numitrs++;

     //initialise the input array here
     for(int i = 0; i<ARRAY_SIZE; i++)
	     input[i]=i;

     sigset_t blockMask, emptyMask;

     //register the signal handler for SIGUSR1 signal
     signal(SIGUSR1,sighandler1);

     //empty the mask and add SIGCHLD and SIGUSR1 to the mask
     sigemptyset(&blockMask);
     sigaddset(&blockMask, SIGCHLD);
     sigaddset(&blockMask, SIGUSR1);

     if (sigprocmask(SIG_SETMASK, &blockMask, NULL) == -1)
     {
	     printf("sigprocmask ERROR");
	     exit(EXIT_FAILURE);
     }
     fflush(stdout);

     //iterate for number of processes
     for(int i=0;i<n;i++)
     {
	     if((cpid=fork())==0)
	     {
		     //child process executes here
		     process_no++;
	
		     if(i==n-1)
		     {
			     //All children created so send signal to main parent so that printing of array can start

			     kill(pid,SIGUSR1);
			     exit(0);
		     }
	     }

	     else
	     {

		     //wait for SIGUSR1  from child processes
		     int itr=0;
		     int index=0;

		     //sigsuspend will wait for SIGUSR1 but not for SIGCHLD
		     sigemptyset(&emptyMask);
		     sigaddset(&emptyMask, SIGCHLD);

		     while(itr<numitrs)
		     {

			     if (sigsuspend(&emptyMask) == -1 && errno != EINTR)
			     {	

				     printf("sigsuspend ERROr");

				     exit(EXIT_FAILURE);
			     }

			     //calculate index of the array that needs to be accessed
			     index= (itr*n)+((process_no)%n);
			     fflush(stdout);

			     //printArray function called for printing the array
			     printArray(input,index);
			     fflush(stdout);


			     if(index == ARRAY_SIZE-1)
				     //The array has been printed completely so start reclaiming the processes.
			     {
				     if(cleanP()<0)
				     {
					     printf("error while cleaning the processes");
					     exit(EXIT_FAILURE);

				     }

			     }

			     itr++;

			     /*go in a roundrobin manner as all the processes have printed 
			      * this sends a signal to the main parent to restart that process*/

			     if(process_no ==(n-1))
			     {
				     printf("\n");

				     kill(pid,SIGUSR1);
			     }	
			     else
				     //send a signal to immediate child
				     kill(cpid,SIGUSR1);


		     }//while ends



	     }//else ends


     }//for ends

}//main ends







