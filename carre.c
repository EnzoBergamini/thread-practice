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

#define TL_CORNER "\xe2\x94\x8c"
#define TR_CORNER "\xe2\x94\x90"
#define BL_CORNER "\xe2\x94\x94"
#define BR_CORNER "\xe2\x94\x98"
#define H_BARRE "\xe2\x94\x80"
#define V_BARRE "\xe2\x94\x82"
#define SPACE " "
#define NEWLINE "\n"

#define MAX_SIZE 10
#define TRUE 1
#define FALSE 0

typedef struct shared_arg_t {
    int nb_thread_pret;

    char *to_print;
    int print_iter;

    int stop;

    pthread_cond_t cond_inter_thread;
    pthread_cond_t cond_principale;
    pthread_mutex_t mutex;
} print_t;

typedef struct arg_t {
    int num_thread;
    int delai_max;
    int nb_thread;

    struct shared_arg_t *shared_arg;
} arg_t;

int alea (int max, unsigned int *seed){
    return max * (rand_r (seed) / ((double) RAND_MAX + 1)) ;
}

void *printer (void *arg){

    /* Récupération des arguments */

    arg_t a = *(arg_t *) arg;

    int delai_max = a.delai_max;
    unsigned int seed = a.num_thread;
    int nb_thread = a.nb_thread;

    usleep (alea(delai_max, &seed) * 1000);

    /* Début du thread et synchronisation de départ*/

    pthread_mutex_lock(&a.shared_arg->mutex); // <======== LOCK
    a.shared_arg->nb_thread_pret += 1;
    pthread_cond_broadcast(&a.shared_arg->cond_inter_thread);

    while (a.shared_arg->nb_thread_pret < nb_thread){
        pthread_cond_wait(&a.shared_arg->cond_inter_thread, &a.shared_arg->mutex);
    }

    printf("thread prêt\n");

    a.shared_arg->nb_thread_pret += 1;
    pthread_mutex_unlock(&a.shared_arg->mutex); // <======== UNLOCK
    pthread_cond_signal(&a.shared_arg->cond_principale);


    /* Boucle principale du thread */

    while(a.shared_arg->stop == FALSE){

        pthread_mutex_lock(&a.shared_arg->mutex); // <======== LOCK
        while (a.shared_arg->print_iter <= 0 && a.shared_arg->stop == FALSE){
            pthread_cond_wait(&a.shared_arg->cond_inter_thread, &a.shared_arg->mutex);
        }

        if (a.shared_arg->stop == TRUE && a.shared_arg->print_iter <= 0){/* si c'est le dernier à écrire */
            pthread_mutex_unlock(&a.shared_arg->mutex); // <======== UNLOCK
            break;
        }

        a.shared_arg->print_iter -= 1;

        printf("%s", a.shared_arg->to_print);

        if (a.shared_arg->print_iter == 0){ /* si c'est le dernier à écrire alors il signal le thread principal */
            pthread_cond_signal(&a.shared_arg->cond_principale);
        }

        pthread_mutex_unlock(&a.shared_arg->mutex); // <======== UNLOCK
        pthread_cond_broadcast(&a.shared_arg->cond_inter_thread);
        usleep (alea(delai_max, &seed) * 1000) ;
    }

    printf("thread terminé\n");

    return NULL;
}

int main(int argc, char *argv[]){

    /* Vérification des arguments */

    if (argc != 4){
        raler(0, "usage: carre <délai-max> <nb-threads> <taille-côté>\n");
    }

    int delai_max = atoi(argv[1]);
    int nb_threads = atoi(argv[2]);
    int taille_cote = atoi(argv[3]);

    if (delai_max < 0 || nb_threads <= 0 || taille_cote < 0){
        raler(0, "usage: carre <délai-max> <nb-threads> <taille-côté>\n");
    }

    printf("début\n");

    /* Initialisation des variables partagées */

    struct shared_arg_t shared_arg;

    shared_arg.nb_thread_pret = 0;
    shared_arg.to_print = calloc(MAX_SIZE, sizeof(char));
    shared_arg.print_iter = 0;
    shared_arg.stop = FALSE;

    TCHK(pthread_cond_init(&shared_arg.cond_inter_thread, NULL));
    TCHK(pthread_cond_init(&shared_arg.cond_principale, NULL));

    TCHK(pthread_mutex_init(&shared_arg.mutex, NULL));

    /* Initialisation des arguments pour les threads */

    arg_t *arg;
    arg = calloc (nb_threads, sizeof (arg_t));

    for (int i = 0; i < nb_threads; ++i) {
        arg[i].num_thread = i;
        arg[i].delai_max = delai_max;
        arg[i].nb_thread = nb_threads;

        arg[i].shared_arg = &shared_arg;
    }

    /* Initialisation des threads */

    pthread_t *tid ;
    tid = calloc (nb_threads, sizeof (pthread_t));


    for (int i = 0; i < nb_threads; ++i) {
        TCHK(pthread_create(&tid[i], NULL, printer, &arg[i]));
    }


    /* Synchronisation de départ */

    pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK
    while(shared_arg.nb_thread_pret < 2*nb_threads){
        pthread_cond_wait(&shared_arg.cond_principale, &shared_arg.mutex);
    }

    pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK

    for (int i = 0; i < taille_cote + 2; ++i) {
        if (i == 0){ /* cas particulier pour la première ligne */
            for (int j = 0; j < 3; j++){
                pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK
                if (j == 0){
                    strncpy(shared_arg.to_print, TL_CORNER, MAX_SIZE);
                    shared_arg.print_iter = 1;
                }else if (j == 2){
                    strncpy(shared_arg.to_print, TR_CORNER, MAX_SIZE);
                    shared_arg.print_iter = 1;
                }else{
                    strncpy(shared_arg.to_print, H_BARRE, MAX_SIZE);
                    shared_arg.print_iter = taille_cote;
                }
                pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK
                pthread_cond_broadcast(&shared_arg.cond_inter_thread);

                pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK
                while (shared_arg.print_iter > 0){
                    pthread_cond_wait(&shared_arg.cond_principale, &shared_arg.mutex);
                }
                pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK
            }
        }else if (i == taille_cote + 1){ /* cas particulier pour la dernière ligne */
            for (int j = 0; j < 3; j++){
                pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK
                if (j == 0){
                    strncpy(shared_arg.to_print, BL_CORNER, MAX_SIZE);
                    shared_arg.print_iter = 1;
                }else if (j == 2){
                    strncpy(shared_arg.to_print, BR_CORNER, MAX_SIZE);
                    shared_arg.print_iter = 1;
                }else{
                    strncpy(shared_arg.to_print, H_BARRE, MAX_SIZE);
                    shared_arg.print_iter = taille_cote;
                }
                pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK
                if (shared_arg.print_iter == 1){
                    pthread_cond_signal(&shared_arg.cond_inter_thread);
                } else{
                    pthread_cond_broadcast(&shared_arg.cond_inter_thread);
                }

                pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK
                while (shared_arg.print_iter > 0){
                    pthread_cond_wait(&shared_arg.cond_principale, &shared_arg.mutex);
                }
                pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK
            }
        }else{ /* cas pour toutes les autres ligne par default */
            for (int j = 0; j < 3; j++){
                pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK
                if (j == 0 || j == 2){
                    strncpy(shared_arg.to_print, V_BARRE, MAX_SIZE);
                    shared_arg.print_iter = 1;
                }else{
                    strncpy(shared_arg.to_print, SPACE, MAX_SIZE);
                    shared_arg.print_iter = taille_cote;
                }
                pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK
                if (shared_arg.print_iter == 1){
                    pthread_cond_signal(&shared_arg.cond_inter_thread);
                } else{
                    pthread_cond_broadcast(&shared_arg.cond_inter_thread);
                }

                pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK
                while (shared_arg.print_iter > 0){
                    pthread_cond_wait(&shared_arg.cond_principale, &shared_arg.mutex);
                }
                pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK
            }
        }

        pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK

        strncpy(shared_arg.to_print, NEWLINE, MAX_SIZE);
        shared_arg.print_iter = 1;

        pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK

        pthread_cond_signal(&shared_arg.cond_inter_thread);

        pthread_mutex_lock(&shared_arg.mutex); // <======== LOCK

        while (shared_arg.print_iter > 0) {
            pthread_cond_wait(&shared_arg.cond_principale, &shared_arg.mutex);
        }
        if (i == taille_cote + 1){
            shared_arg.stop = TRUE;
            pthread_cond_broadcast(&shared_arg.cond_inter_thread);
        }
        pthread_mutex_unlock(&shared_arg.mutex); // <======== UNLOCK

    }

    /* Synchronisation de fin */

    for (int i = 0; i < nb_threads; ++i) {
        TCHK(pthread_join(tid[i], NULL));
    }

    /* Destruction et libération des variables */

    TCHK(pthread_cond_destroy(&shared_arg.cond_inter_thread));
    TCHK(pthread_cond_destroy(&shared_arg.cond_principale));
    TCHK(pthread_mutex_destroy(&shared_arg.mutex));

    free(tid);
    free(shared_arg.to_print);
    free(arg);

    return 0;
}