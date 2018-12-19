#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include<errno.h>
#include<signal.h>
#include<unistd.h>
#define KEY 12345

//number of processes/philosophers
int n = 0;
//flag to check if SIGINT was generated
int siggen = 0;
//save parent process id. This process is later used for clearing up the semaphores.
pid_t pid = 0;

//Semaphore union used along with semctl to initialize the semaphores
union semun
{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
  struct seminfo *__buf;
};

//Function used by process to remove the semaphores once SIGINT is generated.
void
removesemaphores (int semid)
{
  /* Remove the semaphore */
  if (semctl (semid, 0, IPC_RMID) == -1)
    {
      printf ("Semrm failed!");
      exit (EXIT_FAILURE);
    }

  printf ("Semaphores removed successfully!\n");
}

//signal handler for SIGINT signal
static void
sighandler1 (int sig)
{
  if (pid == getpid ())
    {
      siggen = 1;

    }
  else
    exit (EXIT_FAILURE);

}

int main()
{

//registering signal handler
  signal (SIGINT, sighandler1);
  //number of philosophers or processes
  printf ("Enter number of philosophers:");
  scanf ("%d", &n);

  pid = getpid ();
//create the semaphore set
  int semid = semget (KEY, n, IPC_CREAT | 0666);

//loop through the set and initialize them.
  union semun arg;
  for (int i = 0; i < n; i++)
    {
      arg.val = 1;
/* Initialize semaphore */
      if (semctl (semid, i, SETVAL, arg) == -1)
	{
	  printf ("Semctl failed for  %d\n", i);
	  exit (1);
	}
    }
  printf ("Values set for semaphore!\n");
  fflush (stdout);

//semaphore initialization done now we can move on to create n processes
//unique sequential index for all created processes starting from 0
  int process_no = 0;
  pid_t cpid;

  for (int i = 0; i < n; i++)
    {
      if ((cpid = fork ()) == 0)
	{
	  //child process executes here
	  process_no++;
	}
      else
	{
	  struct sembuf getfork[2];
	//structure to lock left fork/semaphore of a process
	  getfork[0].sem_num = process_no % n;
	  getfork[0].sem_op = -1;
	  getfork[0].sem_flg = SEM_UNDO;

         //structure to lock right fork/semaphore of a process
	  getfork[1].sem_num = (process_no + 1) % n;
	  getfork[1].sem_op = -1;
	  getfork[1].sem_flg = SEM_UNDO;

	  struct sembuf releasefork[2];
 //structure to release left fork/semaphore of a process
	  releasefork[0].sem_num = process_no % n;
	  releasefork[0].sem_op = 1;
	  releasefork[0].sem_flg = SEM_UNDO;

 //structure to release right fork/semaphore of a process
	  releasefork[1].sem_num = (process_no + 1) % n;
	  releasefork[1].sem_op = 1;
	  releasefork[1].sem_flg = SEM_UNDO;

	  printf ("Philosopher %d (%d) is Thinking!\n", process_no,getpid ());
	  fflush (stdout);
	  sleep(4);

	  while (1)
	    {

		//time to eat..
		//grab both the fork 
	      if (semop (semid, getfork, 2) > -1)
		{
		  printf ("Philosopher %d (%d) is Eating!\n", process_no,
			  getpid ());
		  sleep (4);
		}
	      else
		{
		  printf ("Error in doing semop");
		  exit (EXIT_FAILURE);
		}

		//done eating now release both the fork
	      if (semop (semid, releasefork, 2) > -1)
		{
                //fork released successfully 
		  printf ("Philosopher %d (%d) is Thinking!\n", process_no,
			  getpid ());
		  sleep (2);
		}
	      else
		{
		  printf ("Error in doing semop");
		  exit (EXIT_FAILURE);
		}
		
		//Detect if the signal was generated,if yes then remove the semaphores. 
	      if (siggen == 1)
		{
		  removesemaphores (semid);
		  exit (EXIT_SUCCESS);

		}

	    }

	}

    }
return 0;
}
