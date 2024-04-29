#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

void semwait ( int semid, int whichsem )
{
   struct sembuf sb;

   sb.sem_num = whichsem;
   sb.sem_flg = 0;
   sb.sem_op = -1;
   semop(semid, &sb, 1);
}

void semsignal ( int semid, int whichsem )
{
   struct sembuf sb;

   sb.sem_num = whichsem;
   sb.sem_flg = 0;
   sb.sem_op = 1;
   semop(semid, &sb, 1);
}

int main ( int argc, char *argv[] )
{
   int *A, *T, n, w, i, *idx;
   int adjid, topid, idxid, mtxid, synid, ntfid;
   key_t adjkey, topkey, idxkey, mtxkey, synkey, ntfkey;

   if (argc > 2) {
      n = atoi(argv[1]);
      w = atoi(argv[2]);
   } else {
      fprintf(stderr, "!!! Usage: worker <No_of_workers> <worker_id>\n");
      exit(1);
   }

   adjkey = ftok("/home/", 'A');
   adjid = shmget(adjkey, n * n * sizeof(int), 0777);
   A = (int *)shmat(adjid, 0, 0);

   topkey = ftok("/home/", 'T');
   topid = shmget(topkey, n * sizeof(int), 0777);
   T = (int *)shmat(topid, 0, 0);

   idxkey = ftok("/home/", 'I');
   idxid = shmget(idxkey, sizeof(int), 0777);
   idx = (int *)shmat(idxid, 0, 0);

   mtxkey = ftok("/home/", 'M');
   mtxid = semget(mtxkey, 1, 0777);

   ntfkey = ftok("/home/", 'N');
   ntfid = semget(ntfkey, 1, 0777);

   synkey = ftok("/home/", 'S');
   synid = semget(synkey, n, 0777);

   for (i=0; i<n; ++i)
      if (A[n*i+w] == 1) semwait(synid, w);

   semwait(mtxid, 0);
   T[*idx] = w;
   ++(*idx);
   semsignal(mtxid, 0);

   for (i=0; i<n; ++i) if (A[n*w+i] == 1) semsignal(synid, i);

   semsignal(ntfid, 0);

   exit(0);
}
