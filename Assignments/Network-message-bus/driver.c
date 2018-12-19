#include<stdio.h>
#include"nmb.c"

int
main (int argc, char const *argv[])
{

  struct msg p;
  printf ("Enter 1 to send a message or  2 to recieve a message\n");
  char ip[16];
  char message[1024] = { 0 };
  unsigned short port = 0;
  int choice = 0;

  scanf ("%d", &choice);
  if (choice == 1)
    {
      printf ("Enter the destination ip: ");
      scanf ("%s", ip);
      ip[15] = '\0';
      char c=0;
      printf ("Enter the destination port: ");
      scanf ("%hd", &port);
      scanf ("%c",&c);
      printf ("Enter the message:");
      fgets (message, 1024, stdin);
      msgsnd_nmb (ip, port, message);
    }
  else if (choice == 2)
    {
      printf ("Enter the source ip/your ip:");

      scanf ("%s", ip);
      ip[15] = '\0';

      printf ("Enter your port: ");
      scanf ("%hd", &port);
      p = msgrcv_nmb (ip, port);
      printf ("%s", p.pload.data);

    }
  else
    {

      printf ("Enter the correct choice!\n");

    }

  return 0;

}
