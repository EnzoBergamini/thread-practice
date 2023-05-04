#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/wait.h>


void raler (int syserr, const char *msg, ...)
{
    va_list ap;


    va_start (ap, msg);
    vfprintf (stderr, msg, ap);
    fprintf (stderr, "\n");
    va_end (ap);

    if (syserr == 1){
        perror ("");
    }

    exit (EXIT_FAILURE);
}

#define CHK(op)  do { if ((op) == -1)   raler (1, #op);} while (0)
#define CHKP(op) do { if ((op) == NULL) raler (1, #op);} while (0)

int main(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr, "Usage: %s <délai-max> <nb-threads> <taille-côté>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}