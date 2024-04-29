#ifndef _FOOTHREAD_H
#define _FOOTHREAD_H

#define FOOTHREAD_THREADS_MAX 100
#define FOOTHREAD_DEFAULT_STACK_SIZE 2097152
#define FOOTHREAD_JOINABLE 1
#define FOOTHREAD_DETACHED 2

typedef struct {
   int tid;
   int jointype;
   int joinsemid;
} foothread_t;

typedef struct {
   int jointype;
   int stacksize;
} foothread_attr_t;

#define FOOTHREAD_ATTR_INITIALIZER (foothread_attr_t){FOOTHREAD_DETACHED, FOOTHREAD_DEFAULT_STACK_SIZE}
void foothread_attr_setjointype ( foothread_attr_t * , int ) ;
void foothread_attr_setstacksize ( foothread_attr_t * , int ) ;

void foothread_create ( foothread_t * , foothread_attr_t * , int (*)(void *) , void * ) ;
void foothread_exit ( ) ;

typedef struct {
   int val;
   int mtxsemid;
   int ltid;
} foothread_mutex_t;

void foothread_mutex_init ( foothread_mutex_t * ) ;
void foothread_mutex_lock ( foothread_mutex_t * ) ;
void foothread_mutex_unlock ( foothread_mutex_t * ) ;
void foothread_mutex_destroy ( foothread_mutex_t * ) ;

typedef struct {
   int ntot;
   int nwait;
   foothread_mutex_t barmtx;
   int barsemid;
} foothread_barrier_t;

void foothread_barrier_init ( foothread_barrier_t * , int ) ;
void foothread_barrier_wait ( foothread_barrier_t * ) ;
void foothread_barrier_destroy ( foothread_barrier_t * ) ;

#endif
