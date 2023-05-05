#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>


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
#define CHKN(op) do { if ((op) == NULL) raler (1, #op);} while (0)
#define TCHK(op) do { if ((errno = (op)) > 0) raler(1, #op); } while (0)

typedef struct arg_t {
    int num_thread;
    int delai_max;
    pthread_cond_t *cond;
    int *nb_thread_pret;
    pthread_mutex_t *mutex;
    int nb_thread;
} arg_t;

int alea (int max, unsigned int *seed)
{
    return max * (rand_r (seed) / ((double) RAND_MAX + 1)) ;
}

void *fonction (void *arg){
    arg_t a = *(arg_t *) arg;
    int delai_max = a.delai_max;
    int num_thread = a.num_thread;
    unsigned int seed = num_thread;
    int delai = alea(delai_max, &seed);

    printf("Je suis le thread %d j'attends %d secondes\n", num_thread ,delai);
    sleep(delai);

    pthread_mutex_lock(a.mutex);
    *(a.nb_thread_pret) += 1;
    pthread_cond_broadcast(a.cond);
    while (*(a.nb_thread_pret) < a.nb_thread){
        pthread_cond_wait(a.cond, a.mutex);
    }
    pthread_mutex_unlock(a.mutex);
    printf("thread %d prêt\n", num_thread);


    return NULL;
}

int main(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr, "Usage: %s <délai-max> <nb-threads> <taille-côté>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int delai_max = atoi(argv[1]);
    int nb_threads = atoi(argv[2]);
//    int taille_cote = atoi(argv[3]);

    int nb_thread_pret = 0;

    pthread_t *tid ;
    tid = calloc (nb_threads, sizeof (pthread_t));

    arg_t *arg;
    arg = calloc (nb_threads, sizeof (arg_t));

    pthread_cond_t cond;
    TCHK(pthread_cond_init(&cond, NULL));

    pthread_mutex_t mutex;
    TCHK(pthread_mutex_init(&mutex, NULL));

    //initialisation
    for (int i = 0; i < nb_threads; ++i) {
        arg[i].num_thread = i;
        arg[i].nb_thread = nb_threads;
        arg[i].delai_max = delai_max;
        arg[i].cond = &cond;
        arg[i].nb_thread_pret = &nb_thread_pret;
        arg[i].mutex = &mutex;
    }

    for (int i = 0; i < nb_threads; ++i) {
        TCHK(pthread_create(&tid[i], NULL, fonction, &arg[i]));
    }

    printf("début\n");

    for (int i = 0; i < nb_threads; ++i) {
        TCHK(pthread_join(tid[i], NULL));
    }

    TCHK(pthread_cond_destroy(&cond));
    TCHK(pthread_mutex_destroy(&mutex));

    free(tid);
    free(arg);

    return 0;
}