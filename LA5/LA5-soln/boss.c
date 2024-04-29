#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

int main ( int argc, char *argv[] )
{
   char fname[256];
   int *A, *T, n, i, j, *idx, badpair;
   FILE *fp;
   int adjid, topid, idxid, mtxid, synid, ntfid;
   key_t adjkey, topkey, idxkey, mtxkey, synkey, ntfkey;
   struct sembuf sb;

   strcpy(fname, (argc > 1) ? argv[1] : "graph.txt");
   fp = (FILE *)fopen(fname, "r");
   fscanf(fp, "%d", &n);

   adjkey = ftok("/home/", 'A');
   adjid = shmget(adjkey, n * n * sizeof(int), 0777 | IPC_CREAT);
   A = (int *)shmat(adjid, 0, 0);

   topkey = ftok("/home/", 'T');
   topid = shmget(topkey, n * sizeof(int), 0777 | IPC_CREAT);
   T = (int *)shmat(topid, 0, 0);

   idxkey = ftok("/home/", 'I');
   idxid = shmget(idxkey, sizeof(int), 0777 | IPC_CREAT);
   idx = (int *)shmat(idxid, 0, 0);

   mtxkey = ftok("/home/", 'M');
   mtxid = semget(mtxkey, 1, 0777 | IPC_CREAT);
   semctl(mtxid, 0, SETVAL, 1);

   ntfkey = ftok("/home/", 'N');
   ntfid = semget(ntfkey, 1, 0777 | IPC_CREAT);
   semctl(ntfid, 0, SETVAL, 0);

   synkey = ftok("/home/", 'S');
   synid = semget(synkey, n, 0777 | IPC_CREAT);
   for (i=0; i<n; ++i) semctl(synid, i, SETVAL, 0);
   for (i=0; i<n; ++i) for (j=0; j<n; ++j) fscanf(fp, "%d", &A[n*i + j]);

   *idx = 0;

   shmdt(idx);

   printf("+++ Boss: Setup done...\n");

   sb.sem_num = 0; sb.sem_flg = 0; sb.sem_op = -1;

   for (i=0; i<n; ++i) semop(ntfid, &sb, 1);

   printf("+++ Topological sorting of the vertices\n");
   for (i=0; i<n; ++i) printf("%-4d", T[i]);
   printf("\n");

   badpair = 0;
   for (i=1; i<n; ++i) {
      for (j=0; j<i; ++j) {
         if (A[n * T[i] + T[j]] == 1) {
            printf(">>> Why is %d listed before %d?\n", j, i);
            ++badpair;
         }
      }
   }
   if (badpair) printf("+++ There are %d bad pairs in the listing\n", badpair);
   else printf("+++ Boss: Well done, my team...\n");

   shmdt(A);
   shmdt(T);

   shmctl(adjid, IPC_RMID, 0);
   shmctl(topid, IPC_RMID, 0);
   shmctl(idxid, IPC_RMID, 0);

   semctl(mtxid, 0, IPC_RMID, 0);
   semctl(ntfid, 0, IPC_RMID, 0);
   semctl(synid, 0, IPC_RMID, 0);

   exit(0);
}
