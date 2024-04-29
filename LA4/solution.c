#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void produce ( int n, int t, int shmid )
{
   int *M, i, c, item;
   int *cnt, *sum;

   M = (int *)shmat(shmid, 0, 0);
   printf("Producer is alive\n");

   cnt = (int *)malloc((n+1) * sizeof(int));
   sum = (int *)malloc((n+1) * sizeof(int));
   for (i=0; i<=n; ++i) cnt[i] = sum[i] = 0;

   for (i=0; i<t; ++i) {
      c = 1 + rand() % n;
      item = 100 + rand() % 899;
      #ifdef VERBOSE
      printf("Producer produces %d for Consumer %d\n", item, c);
      #endif
      M[0] = c;
      #ifdef SLEEP
      usleep(1);
      #endif
      M[1] = item;
      ++cnt[0]; ++cnt[c]; sum[c] += item;
      while (M[0] != 0) { }
   }

   M[0] = -1;

   for (i=1; i<=n; ++i) wait(NULL);

   printf("Producer has produced %d items\n", cnt[0]);
   for (i=1; i<=n; ++i)
   printf("%d items for Consumer %d: Checksum = %d\n", cnt[i], i, sum[i]);

   free(cnt); free(sum);

   shmdt(M);

   shmctl(shmid, IPC_RMID, 0);
}

void consume ( int i, int shmid )
{
   int *M;
   int nitem = 0;
   int sum = 0;

   M = (int *)shmat(shmid, 0, 0);
   printf("\t\t\t\t\t\tConsumer %d is alive\n", i);

   while (1) {
      while ( (M[0] != -1) && (M[0] != i) ) { }
      if (M[0] == -1) break;
      #ifdef VERBOSE
      printf("\t\t\t\t\t\tConsumer %d reads %d\n", i, M[1]);
      #endif
      sum += M[1];
      M[0] = 0;
      ++nitem;
   }

   printf("\t\t\t\t\t\tConsumer %d has read %d items: Checksum = %d\n", i, nitem, sum);

   shmdt(M);

   exit(0);
}

int main ( int argc, char *argv[] )
{
   int n, t, i;
   int shmid;
   int *M;

   srand((unsigned int)time(NULL));
   n = (argc > 1) ? atoi(argv[1]) : 1;
   t = (argc > 2) ? atoi(argv[2]) : 10;

   printf("n = %d\nt = %d\n", n, t);

   shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0777 | IPC_CREAT);
   M = (int *)shmat(shmid, 0, 0);
   M[0] = M[1] = 0;

   for (i=1; i<=n; ++i) {
      if (!fork()) consume(i,shmid);
   }

   produce(n,t,shmid);

   exit(0);
}