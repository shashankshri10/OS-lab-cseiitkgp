#include <stdio.h>
#include <stdlib.h>
#include <foothread.h>

int n;
int *P, *S;
int *arg;
foothread_mutex_t M;
foothread_barrier_t *B;

int follower ( void * ) ;

void readtree ( )
{
   FILE *fp;
   int i, j, k;

   fp = (FILE *)fopen("tree.txt", "r");
   fscanf(fp, "%d", &n);
   P = (int *)malloc(n * sizeof(int));
   for (i=0; i<n; ++i) {
      fscanf(fp, "%d%d", &j, &k);
      P[j] = k;
   }
   fclose(fp);
   S = (int *)malloc(n * sizeof(int));
   for (i=0; i<n; ++i) S[i] = 0;
}

void initresources ( )
{
   int *nb, i, j;

   foothread_mutex_init(&M);

   B = (foothread_barrier_t *)malloc(n * sizeof(foothread_barrier_t));

   nb = (int *)malloc(n * sizeof(int));
   for (i=0; i<n; ++i) nb[i] = 1;
   for (i=0; i<n; ++i) {
      j = P[i];
      if (j != i) ++nb[j];
   }

   for (i=0; i<n; ++i) foothread_barrier_init(&B[i], nb[i]);

   free(nb);
}

void createfollowers ( )
{
   int i;
   foothread_attr_t attr = FOOTHREAD_ATTR_INITIALIZER;

   arg = (int *)malloc(n * sizeof(int));

   foothread_attr_setjointype(&attr, FOOTHREAD_JOINABLE);
   for (i=0; i<n; ++i) {
      arg[i] = i;
      foothread_create(NULL, &attr, follower, (void *)(arg + i));
   }
}

void cleanup ( )
{
   int i;

   free(P);
   free(S);
   free(arg);
   foothread_mutex_destroy(&M);
   for (i=0; i<n; ++i) foothread_barrier_destroy(&B[i]);
   free(B);
}

int follower ( void *arg )
{
   int i;

   i = *((int *)arg);
   foothread_barrier_wait(&B[i]);
   foothread_mutex_lock(&M);
   if (S[i] == 0) {
      printf("Leaf node %3d :: Enter a positive integer: ", i);
      scanf("%d", &S[i]);
   } else {
      printf("Internal node %3d gets the partial sum %d from its children\n", i, S[i]);
   }
   if (P[i] != i) S[P[i]] += S[i];
   foothread_mutex_unlock(&M);
   if (P[i] != i) foothread_barrier_wait(&B[P[i]]);
   else {
      printf("Sum at root (node %d) = %d\n", i, S[i]);
   }
   foothread_exit();
   return 0;
}

int main ()
{
   readtree();
   initresources();
   createfollowers();
   foothread_exit();
   cleanup();
   exit(0);
}
