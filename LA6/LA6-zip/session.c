#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <event.h>

typedef struct {
   int vno;
   int cno;
   int dur;
} visrec;

pthread_mutex_t mtxD = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxP = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxR = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxS = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtxT = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cndD = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cndP = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cndR = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cndS = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cndT = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barD;
pthread_barrier_t barV;
int sessionover;
int currtime;
int doctoravl;
int visno;
int waiting = 0;

void prnclocktime ( char *btxt, int t, char *atxt )
{
   int h, m;

   h = t / 60;
   m = t % 60;
   if (m < 0) { h -= 1; m += 60; }
   h += 9;
   printf("%s", btxt);
   if (h < 12) printf("%02d:%02dam", h, m);
   else if (h == 12) printf("12:%02dpm", m);
   else printf("%02d:%02dpm", h-12, m);
   printf("%s", atxt);
}

void *doctor ( void *arg )
{
   while (1) {
      /* Wait until ready for the next visitor */
      pthread_mutex_lock(&mtxD);
      waiting = 1;
      pthread_cond_wait(&cndD,&mtxD);
      if (sessionover) {
         pthread_mutex_lock(&mtxT);
         prnclocktime("[", currtime, "] Doctor leaves\n");
         pthread_mutex_unlock(&mtxT);
         pthread_mutex_unlock(&mtxD);
         pthread_barrier_wait(&barD);
         break;
      } else {
         pthread_mutex_unlock(&mtxD);
      }

      /* Attend to the next visitor */
      pthread_mutex_lock(&mtxT);
      prnclocktime("[", currtime, "] Doctor has next visitor\n");
      pthread_mutex_unlock(&mtxT);

   }
   pthread_exit(NULL);
}

void *patient ( void *arg )
{
   int n, nv;
   visrec *vr;

   vr = (visrec *)arg;
   n = vr -> cno;
   nv = vr -> vno;

   pthread_mutex_lock(&mtxP);
   waiting = 1;
   pthread_cond_wait(&cndP, &mtxP);
   pthread_mutex_unlock(&mtxP);

   pthread_mutex_lock(&mtxT);
   prnclocktime("[", currtime, " ‒ ");
   prnclocktime("", currtime + (vr -> dur), "] ");
   printf("Patient %d is in doctor's chamber\n", n);
   pthread_mutex_unlock(&mtxT);

   visno = nv;
   pthread_barrier_wait(&barV);
   
   pthread_exit(NULL);
}

void *reporter ( void *arg )
{
   int n, nv;
   visrec *vr;

   vr = (visrec *)arg;
   n = vr -> cno;
   nv = vr -> vno;

   pthread_mutex_lock(&mtxR);
   waiting = 1;
   pthread_cond_wait(&cndR,&mtxR);
   pthread_mutex_unlock(&mtxR);

   pthread_mutex_lock(&mtxT);
   prnclocktime("[", currtime, " ‒ ");
   prnclocktime("", currtime + (vr -> dur), "] ");
   printf("Reporter %d is in doctor's chamber\n", n);
   pthread_mutex_unlock(&mtxT);
   
   visno = nv;
   pthread_barrier_wait(&barV);
   
   pthread_exit(NULL);
}

void *salesrep ( void *arg )
{
   int n, nv;
   visrec *vr;

   vr = (visrec *)arg;
   n = vr -> cno;
   nv = vr -> vno;

   pthread_mutex_lock(&mtxS);
   waiting = 1;
   pthread_cond_wait(&cndS,&mtxS);
   pthread_mutex_unlock(&mtxS);

   pthread_mutex_lock(&mtxT);
   prnclocktime("[", currtime, " ‒ ");
   prnclocktime("", currtime + (vr -> dur), "] ");
   printf("Sales representative %d is in doctor's chamber\n", n);
   pthread_mutex_unlock(&mtxT);
   
   visno = nv;
   pthread_barrier_wait(&barV);
   
   pthread_exit(NULL);
}

int main ( )
{
   int np, nr, ns, npw, nrw, nsw, nv;
   visrec *vr;
   eventQ E;
   event e;
   pthread_t t;

   sessionover = 0;
   currtime = 0;
   doctoravl = 0;
   np = nr = ns = npw = nrw = nsw = nv = 0;

   vr = (visrec *)malloc(128 * sizeof(visrec));

   E = initEQ("arrival.txt");
   E = addevent(E, (event){'D', 0, 0});

   pthread_create(&t,NULL,doctor,NULL);

   while (!emptyQ(E)) {
      e = nextevent(E);
      E = delevent(E);
      pthread_mutex_lock(&mtxT);
      currtime = e.time;
      if (e.type == 'P') {
         ++np;
         prnclocktime("          [", currtime, "] ");
         printf("Patient %d arrives\n", np);
         if (np > 25) {
            prnclocktime("          [", currtime, "] ");
            if (sessionover)
               printf("Patient %d leaves (session over)\n", np);
            else
               printf("Patient %d leaves (quota full)\n", np);
         } else {
            ++npw;
            vr[nv] = (visrec){nv, np, e.duration};
            pthread_create(&t,NULL,patient,(void *)(&vr[nv]));
            ++nv;
            while (1) {
               pthread_mutex_lock(&mtxP);
               if (waiting == 1) {
                  waiting = 0;
                  pthread_mutex_unlock(&mtxP);
                  break;
               }
               pthread_mutex_unlock(&mtxP);
            }
         }
      } else if (e.type == 'R') {
         ++nr;
         prnclocktime("          [", currtime, "] ");
         printf("Reporter %d arrives\n", nr);
         if (sessionover) {
            prnclocktime("          [", currtime, "] ");
            printf("Reporter %d leaves (session over)\n", nr);
         } else {
            ++nrw;
            vr[nv] = (visrec){nv, nr, e.duration};
            pthread_create(&t,NULL,reporter,(void *)(&vr[nv]));
            ++nv;
            while (1) {
               pthread_mutex_lock(&mtxR);
               if (waiting == 1) {
                  waiting = 0;
                  pthread_mutex_unlock(&mtxR);
                  break;
               }
               pthread_mutex_unlock(&mtxR);
            }
         }
      } else if (e.type == 'S') {
         ++ns;
         prnclocktime("          [", currtime, "] ");
         printf("Sales representative %d arrives\n", ns);
         if (ns > 3) {
            prnclocktime("          [", currtime, "] ");
            if (sessionover)
               printf("Sales representative %d leaves (session over)\n", ns);
            else
               printf("Sales representative %d leaves (quota full)\n", ns);
         } else {
            ++nsw;
            vr[nv] = (visrec){nv, ns, e.duration};
            pthread_create(&t,NULL,salesrep,(void *)(&vr[nv]));
            ++nv;
            while (1) {
               pthread_mutex_lock(&mtxS);
               if (waiting == 1) {
                  waiting = 0;
                  pthread_mutex_unlock(&mtxS);
                  break;
               }
               pthread_mutex_unlock(&mtxS);
            }
         }
      } else if (e.type == 'D') {
         pthread_mutex_lock(&mtxD);
         doctoravl = 1;
         pthread_mutex_unlock(&mtxD);
      } else {
         fprintf(stderr, "*** Error in main thread: Unknown event '%c'\n", e.type);
      }
      pthread_mutex_unlock(&mtxT);

      if ( (doctoravl) && (!sessionover)) {
         if ( (nrw > 0) || (npw > 0) || (nsw > 0) ) {

            pthread_mutex_lock(&mtxD);
            pthread_cond_signal(&cndD);
            pthread_mutex_unlock(&mtxD);

            while (1) {
               pthread_mutex_lock(&mtxD);
               if (waiting == 1) {
                  doctoravl = 0;
                  waiting = 0;
                  pthread_mutex_unlock(&mtxD);
                  break;
               }
               pthread_mutex_unlock(&mtxD);
            }

            pthread_barrier_init(&barV, NULL, 2);
            if (nrw > 0) {
               pthread_mutex_lock(&mtxR);
               --nrw;
               pthread_cond_signal(&cndR);
               pthread_mutex_unlock(&mtxR);
            } else if (npw > 0) {
               pthread_mutex_lock(&mtxP);
               --npw;
               pthread_cond_signal(&cndP);
               pthread_mutex_unlock(&mtxP);
            } else if (nsw > 0) {
               pthread_mutex_lock(&mtxS);
               --nsw;
               pthread_cond_signal(&cndS);
               pthread_mutex_unlock(&mtxS);
            }

            pthread_barrier_wait(&barV);
            pthread_barrier_destroy(&barV);

            pthread_mutex_lock(&mtxT);
            E = addevent(E, (event){'D', currtime + vr[visno].dur, 0});
            pthread_mutex_unlock(&mtxT);

         } else if ( (np >= 25) && (ns >= 3) ) {
            pthread_barrier_init(&barD, NULL, 2);
            pthread_mutex_lock(&mtxD);
            doctoravl = 0;
            sessionover = 1;
            pthread_cond_signal(&cndD);
            pthread_mutex_unlock(&mtxD);
            pthread_barrier_wait(&barD);
            pthread_barrier_destroy(&barD);
         }
      }
   }

   pthread_exit(NULL);
}
