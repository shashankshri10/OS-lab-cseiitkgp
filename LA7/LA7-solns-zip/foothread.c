#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <foothread.h>

static int _FOOTHREAD_THREADS_CNT = 0;
static foothread_t _FOOTHREAD_THREADS_TABLE[FOOTHREAD_THREADS_MAX];
static foothread_mutex_t _FOOTHREAD_SYS_MUTEX;

void foothread_create ( foothread_t *footid, foothread_attr_t *attr, int (*tmain)(void *) , void *arg )
{
   void *stack;
   int jointype, stacksize;

   if (_FOOTHREAD_THREADS_CNT == FOOTHREAD_THREADS_MAX) {
      fprintf(stderr, "*** foothread_create: Unable to create new thread\n");
      return;
   }

   if (attr == NULL) {
      jointype = FOOTHREAD_DETACHED;
      stacksize = FOOTHREAD_DEFAULT_STACK_SIZE;
   } else {
      jointype = ((attr -> jointype) == FOOTHREAD_JOINABLE) ? FOOTHREAD_JOINABLE : FOOTHREAD_DETACHED;
      stacksize = attr -> stacksize;
   }

   if (_FOOTHREAD_THREADS_CNT == 0) {
      foothread_mutex_init(&_FOOTHREAD_SYS_MUTEX);
   }

   foothread_mutex_lock(&_FOOTHREAD_SYS_MUTEX);
   stack = (void *)malloc(stacksize);
   _FOOTHREAD_THREADS_TABLE[_FOOTHREAD_THREADS_CNT].tid =
      clone(tmain, stack + stacksize, CLONE_FILES | CLONE_FS | CLONE_SIGHAND | CLONE_THREAD | CLONE_VM, arg);
   if (jointype == FOOTHREAD_JOINABLE) {
      _FOOTHREAD_THREADS_TABLE[_FOOTHREAD_THREADS_CNT].jointype = FOOTHREAD_JOINABLE;
      _FOOTHREAD_THREADS_TABLE[_FOOTHREAD_THREADS_CNT].joinsemid = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
      semctl(_FOOTHREAD_THREADS_TABLE[_FOOTHREAD_THREADS_CNT].joinsemid, 0, SETVAL, 0);
   } else {
      _FOOTHREAD_THREADS_TABLE[_FOOTHREAD_THREADS_CNT].jointype = FOOTHREAD_DETACHED;
      _FOOTHREAD_THREADS_TABLE[_FOOTHREAD_THREADS_CNT].joinsemid = -1;
   }
   if (footid) *footid = _FOOTHREAD_THREADS_TABLE[_FOOTHREAD_THREADS_CNT];
   ++_FOOTHREAD_THREADS_CNT;
   foothread_mutex_unlock(&_FOOTHREAD_SYS_MUTEX);
   return;
}

void foothread_exit ( )
{
   int tid, i;
   struct sembuf sb;

   tid = gettid();
   if (tid == getpid()) {
      for (i=0; i<_FOOTHREAD_THREADS_CNT; ++i) {
         if (_FOOTHREAD_THREADS_TABLE[i].jointype == FOOTHREAD_JOINABLE) {
            sb.sem_num = 0;
            sb.sem_flg = 0;
            sb.sem_op = -1;
            semop(_FOOTHREAD_THREADS_TABLE[i].joinsemid, &sb, 1);
            semctl(_FOOTHREAD_THREADS_TABLE[i].joinsemid, 0, IPC_RMID, 0);
         }
      }
      foothread_mutex_destroy(&_FOOTHREAD_SYS_MUTEX);
   } else {
      for (i=0; i<_FOOTHREAD_THREADS_CNT; ++i) {
         if ((tid == _FOOTHREAD_THREADS_TABLE[i].tid) && (_FOOTHREAD_THREADS_TABLE[i].jointype == FOOTHREAD_JOINABLE)) {
            sb.sem_num = 0;
            sb.sem_flg = 0;
            sb.sem_op = 1;
            semop(_FOOTHREAD_THREADS_TABLE[i].joinsemid, &sb, 1);
            break;
         }
      }
   }
}

void foothread_attr_setjointype ( foothread_attr_t *fa , int jointype )
{
   fa -> jointype = (jointype == FOOTHREAD_JOINABLE) ? FOOTHREAD_JOINABLE : FOOTHREAD_DETACHED;
}

void foothread_attr_setstacksize ( foothread_attr_t *fa , int stacksize )
{
   fa -> stacksize = stacksize;
}

void foothread_mutex_init ( foothread_mutex_t *M )
{
   M -> mtxsemid = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
   semctl(M -> mtxsemid, 0, SETVAL, 1);
   M -> val = 1;
   M -> ltid = -1;
}

void foothread_mutex_lock ( foothread_mutex_t *M )
{
   struct sembuf sb;

   sb.sem_num = 0; sb.sem_flg = 0; sb.sem_op = -1;
   semop(M -> mtxsemid, &sb, 1);
   M -> ltid = gettid();
   M -> val = 0;
}

void foothread_mutex_unlock ( foothread_mutex_t *M )
{
   struct sembuf sb;
   
   if ((M -> val != 0) || (M -> ltid != gettid())) {
      fprintf(stderr, "*** foothread_mutex_unlock: Illegal request\n");
      return;
   }
   sb.sem_num = 0; sb.sem_flg = 0; sb.sem_op = 1;
   M -> val = 1;
   M -> ltid = -1;
   semop(M -> mtxsemid, &sb, 1);
}

void foothread_mutex_destroy ( foothread_mutex_t *M )
{
   semctl(M -> mtxsemid, 0, IPC_RMID, 0);
   M -> mtxsemid = -1;
}

void foothread_barrier_init ( foothread_barrier_t *B, int N )
{
   foothread_mutex_init(&(B -> barmtx));
   B -> ntot = N;
   B -> nwait = 0;
   B -> barsemid = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
   semctl(B -> barsemid, 0, SETVAL, 0);
}

void foothread_barrier_wait ( foothread_barrier_t *B )
{
   struct sembuf sb;
   int i;

   foothread_mutex_lock(&(B -> barmtx));
   ++(B -> nwait);
   if (B -> nwait < B -> ntot) {
      sb.sem_num = 0; sb.sem_flg = 0; sb.sem_op = -1;
      foothread_mutex_unlock(&(B -> barmtx));
      semop(B -> barsemid, &sb, 1);
   } else {
      sb.sem_num = 0; sb.sem_flg = 0; sb.sem_op = 1;
      for (i=1; i<B->ntot; ++i) semop(B -> barsemid, &sb, 1);
      foothread_mutex_unlock(&(B -> barmtx));
   }
}

void foothread_barrier_destroy ( foothread_barrier_t *B )
{
   foothread_mutex_destroy(&(B -> barmtx));
   semctl(B -> barsemid, 0, IPC_RMID, 0);
}
