#include <stdio.h>
#include <unistd.h>
#include <string.h>  /* For strlen             */
#include <mpi.h>     /* For MPI functions, etc */ 

const int MAX_STRING = 1024;

int main(void) {
   char       greeting[MAX_STRING];  /* String storing message */
   int        comm_sz;               /* Number of processes    */
   int        my_rank;               /* My process rank        */


   /* Start up MPI */
   MPI_Init(NULL, NULL); 

   /* Get the number of processes */
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz); 

   /* Get my rank among all the processes */
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 
   
   char hostname[512];
   gethostname(hostname, 512);

   if (my_rank != 0) { 
      /* Create message */
      sprintf(greeting, "Greetings from process %d of %d - %s!", 
            my_rank, comm_sz, hostname);
      /* Send message to process 0 */
      MPI_Send(greeting, strlen(greeting)+1, MPI_CHAR, 0, 0,
            MPI_COMM_WORLD); 
   } else {  
      /* Print my message */
      printf("Greetings from process %d of %d - %s!\n", my_rank, comm_sz, hostname);
      for (int q = 1; q < comm_sz; q++) {
         /* Receive message from process q */
         MPI_Recv(greeting, MAX_STRING, MPI_CHAR, q,
            0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         /* Print message from process q */
         printf("%s\n", greeting);
      } 
   }

   /* Shut down MPI */
   MPI_Finalize(); 

   return 0;
}  /* main */
