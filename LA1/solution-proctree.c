#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILENAME "treeinfo.txt"

void procnode ( char *myname, int level )
{
   FILE *fp;
   int i, nc, namefnd = 0;
   char name[32], cname[32], cc[8], clvl[8], line[256], *cp;
   pid_t mypid, cpid;

   fp = (FILE *)fopen(FILENAME, "r");
   if (fp == NULL) {
      fprintf(stderr, "Unable to open data file\n");
      exit(2);
   }

   mypid = getpid();
   while (1) {
      fgets(line, 256, fp);
      if (feof(fp)) break;
      cp = line;
      sscanf(cp, "%s", name);
      cp += strlen(name) + 1;
      if (!strcmp(name, myname)) {
         for (i=0; i<level; ++i) printf("    ");
         printf("%s (%d)\n", myname, mypid);
         namefnd = 1;
         sscanf(cp, "%s", cc);
         cp += strlen(cc) + 1;
         nc = atoi(cc);
         for (i=0; i<nc; ++i) {
            sscanf(cp, "%s", cname);
            cp += strlen(cname) + 1;
            if ((cpid = fork())) {
               waitpid(cpid, NULL, 0);
            } else {
               sprintf(clvl, "%d", level + 1);
               execlp("./proctree", "./proctree", cname, clvl, NULL);
            }
         }
         break;
      }
   }

   fclose(fp);

   if (!namefnd) {
      fprintf(stderr, "City %s not found\n", myname);
      exit(3);
   }
}

int main ( int argc, char *argv[] )
{
   if (argc == 1) {
      fprintf(stderr, "Run with a node name\n");
      exit(1);
   }

   sleep(1);

   procnode(argv[1] , (argc >= 3) ? atoi(argv[2]) : 0);

   exit(0);
}