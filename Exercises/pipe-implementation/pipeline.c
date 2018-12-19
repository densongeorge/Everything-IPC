
/*Shell program that works for any number of piped arguments
 * */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/wait.h>


//function closes the read end of the pipe and dups the stdout fd to the write end of the pipe.
void write_pipe (int *pfd)
{
  if (close (pfd[0]) == -1)	//child closes the read end of the pipe as it will be writing
    {
      printf ("error when closing the read end of the pipe");
      exit (EXIT_FAILURE);
    }

  if (pfd[1] != STDOUT_FILENO)
    {				//defensive check if pipe write end is connected to stdout
      if (dup2 (pfd[1], STDOUT_FILENO) == -1)
	{
	  printf ("error when duping the write end of the pipe");
	  exit (EXIT_FAILURE);

	}
      if (close (pfd[1]) == -1)
	{
	  printf ("error when closing the duplicate write end of the pipe");
	  exit (EXIT_FAILURE);


	}


    }

}

//function closes the write end of the pipe and dups the stdin fd to the read end of the pipe.

void read_pipe (int *pfd)
{
  if (close (pfd[1]) == -1)	
	  // error while closing the write end of the pipe
    {
      printf ("error when closing the write end of the pipe by child 2");
      exit (EXIT_FAILURE);

    }
  if (pfd[0] != STDIN_FILENO)	
	  // if pipe read end is already connected to stdin no need to do this
    {
      if (dup2 (pfd[0], STDIN_FILENO) == -1)
	{
	  //error in duping
	  printf ("error when duping the read end of the pipe");
	  exit (EXIT_FAILURE);
	}
      if (close (pfd[0]) == -1)
	{
	  printf ("error when closing the now duplicate read fd of the pipe");
	  exit (EXIT_FAILURE);

	}


    }


}

int main (int argc, char *argv[])
{
// commands hold the command
// parameters hold the parameters of the command
// we use an array of such a structure per command that is delimited by a pipe
  typedef struct
  {
    char *commands;
    int count_parameters;
    char *parameters[100];
  } inputStruct;


  //enter user input
  printf ("Please avoid trailing and leading spaces in each pipe delimited command\n");
  printf ("Enter any number of input commands separated by a | \n");
  char input[1000];
  scanf ("%[^\n]s", input);

  //start processing the input now
  char *str, *tofree;
  tofree = str = strdup (input);	// We own str's memory now.
  //count the number of pipes
  int i, count, count_spaces = 0;
  for (i = 0, count = 0; str[i]; i++)
    count += (str[i] == '|');
  
  //separate the string in terms of the pipes here
  char *token[count + 1];

  char *param[count + 1];
  inputStruct ips[count + 1];
  i = 0;
  while (((token[i]) = strsep (&str, "|")))
    {
      i++;
    }
//separate based on spaces and put individual paramters in parameters member of the structure
  for (i = 0; i < (count + 1); i++)
    {


      ips[i].commands = strsep (&token[i], " ");
      if (token[i] != NULL)
	param[i] = token[i];

      if (token[i] != NULL)
	{
	  for (int j = 0, count_spaces = 0; param[i][j]; j++)
	    count_spaces += (param[i][j] == ' ');
	}

      if (token[i] != NULL)
	ips[i].count_parameters = count_spaces + 2;
      else
	ips[i].count_parameters = 1;


      int k = 1;
      ips[i].parameters[0] = ips[i].commands;

      if (ips[i].count_parameters > 1)
	{
	  while (((ips[i].parameters[k]) = strsep (&param[i], " ")))
	    {
	      k++;
	    }
	}

      ips[i].parameters[k] = NULL;
    }

//input string processing ends here

  //no pipes are present in the input so do this.
  if(count==0)
  {
	execvp (ips[0].commands, ips[0].parameters);
	 printf ("No such command %s",ips[0].commands);
	exit(EXIT_FAILURE);
  }

  //continue processing if there is atleast one pipe
  int N = count;
  int k = N - 1;
  int pfd[2 * N];		// declare file descriptors for the pipe

  for (i = 0; i < N; i++)
    {

      if (pipe (&pfd[2 * i]) == -1)	//check for errors while creating pipe
	{
	  printf ("error when creating pipes");
	  exit (EXIT_FAILURE);
	}
    }



  //create a new child process here
  switch (fork ())
    {
    case -1:
      printf ("error when creating child process");
      exit (EXIT_FAILURE);

    case 0:			
      //first child process will execute first command 

      write_pipe (&pfd[2 * 0]);
      execvp (ips[0].commands, ips[0].parameters);	// write to the pipe
      
      // if the exec has returned means error has occured
      printf ("Error in command %s ",ips[0].commands);
      exit (EXIT_FAILURE);

    default:
      break;
    }

  //intermediate commands between the first and the last command  will be executed here
  for (i = 0; i < k; i++)
    {

      switch (fork ())
	{

	case -1:
	  printf ("error when creating child process");
	  exit (EXIT_FAILURE);
	case 0:		
	  read_pipe (&pfd[2 * i]);
	  write_pipe (&pfd[2 * i + 2]);
	  execvp (ips[i + 1].commands, ips[i + 1].parameters);	// read from  the pipe
	  printf ("Error in command %s ",ips[i+1].commands);
	  exit (EXIT_FAILURE);

	default:
	  break;
	}

      if (close (pfd[2 * i]) == -1)
	{
	        printf("ERROR in close pipe %d\n",2*i);
		 exit (EXIT_FAILURE);
	}

      if (close (pfd[2 * i + 1]) == -1)
	{

		printf ("ERROR in close pipe %d\n",2 *i +1);
		 exit (EXIT_FAILURE);

	}

    }

//execute the last command 
  switch (fork ())
    {

    case -1:
      printf ("error when creating child process");
      exit (EXIT_FAILURE);
    case 0:		
      read_pipe (&pfd[2 * (i - 1) + 2]);
      execvp (ips[count].commands, ips[count].parameters);
     
      // if the exec has returned meaning error
      printf ("Error in command %s ",ips[count].commands);
      exit (EXIT_FAILURE);

    default:
      break;
    }



  //Parent closes unused file descriptors and waits for children

  if (close (pfd[2 * (i - 1) + 2]) == -1)
    {
      printf ("ERROR in close pipe %d\n",2 * (i - 1) + 2);
      exit (EXIT_FAILURE);
    }

  if (close (pfd[2 * (i - 1) + 3]) == -1)
    {
      printf ("ERROR in close pipe %d\n",2 * (i - 1) + 3);
      exit (EXIT_FAILURE);
    }

  int status = 0;
  int pid = 0;
  //wait for all children to terminate
  while ((pid = waitpid (-1, &status, 0)) != -1);
free(tofree);
}
