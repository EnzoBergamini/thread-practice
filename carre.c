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

typedef struct arg_t {
    int num_thread;
    int delai_max;
    int *nb_thread_pret;
    int nb_thread;

    char *to_print;
    int *print_iter;
    int *stop;

    pthread_cond_t *cond_inter_thread;
    pthread_cond_t *cond_principale;
    pthread_mutex_t *mutex;
} arg_t;

int alea (int max, unsigned int *seed){
    return max * (rand_r (seed) / ((double) RAND_MAX + 1)) ;
}

void *fonction (void *arg){
    arg_t a = *(arg_t *) arg;
    int delai_max = a.delai_max;
    int num_thread = a.num_thread;
    unsigned int seed = num_thread;

    usleep (alea(delai_max, &seed) * 1000); // on attend un temps aléatoire entre 0 et delai_max au début

    // synchronisation de départ
    pthread_mutex_lock(a.mutex);
    *(a.nb_thread_pret) += 1; // on incrémente le nombre de thread prêt
    pthread_cond_broadcast(a.cond_inter_thread);
    while (*(a.nb_thread_pret) < a.nb_thread){
        pthread_cond_wait(a.cond_inter_thread, a.mutex);
    }
    printf("thread prêt\n");
    *(a.nb_thread_pret) += 1; // on incrémente le nombre de thread prêt pour la boucle principale
    pthread_mutex_unlock(a.mutex);
    pthread_cond_signal(a.cond_principale);

    //boucle principale

    while(*(a.stop) == FALSE){
        pthread_mutex_lock(a.mutex);
        while (*(a.print_iter) <= 0){
            pthread_cond_wait(a.cond_inter_thread, a.mutex);
        }

        if (*(a.stop) == TRUE && *(a.print_iter) <= 0){ // si le programme est terminé on sort de la boucle
            pthread_mutex_unlock(a.mutex);
            break;
        }

        *(a.print_iter) -= 1;
//        printf("Thread %d affiche : %s\n", num_thread,a.to_print);
        printf("%s", a.to_print);
        if (*(a.print_iter) == 0){ // si c'est le dernier thread à afficher on réveille tout le monde
            pthread_cond_signal(a.cond_principale);
        }
        pthread_mutex_unlock(a.mutex);
        pthread_cond_broadcast(a.cond_inter_thread);
        usleep (alea(delai_max, &seed) * 1000) ;
    }

    printf("thread terminé\n");
    return NULL;
}

int main(int argc, char *argv[]){
    if (argc != 4){
        raler(0, "usage: carre <délai-max> <nb-threads> <taille-côté>\n");
    }

    int delai_max = atoi(argv[1]);
    int nb_threads = atoi(argv[2]);
    int taille_cote = atoi(argv[3]);

    if (delai_max < 0 || nb_threads <= 0 || taille_cote < 0){
        raler(0, "usage: carre <délai-max> <nb-threads> <taille-côté>\n");
    }

    int nb_thread_pret = 0;
    int stop = FALSE;

    pthread_t *tid ;
    tid = calloc (nb_threads, sizeof (pthread_t));

    arg_t *arg;
    arg = calloc (nb_threads, sizeof (arg_t));

    char *to_print = calloc(MAX_SIZE, sizeof(char));
    int print_iter = 0;


    pthread_cond_t cond_inter_thread;
    pthread_cond_t cond_principale;

    TCHK(pthread_cond_init(&cond_inter_thread, NULL));
    TCHK(pthread_cond_init(&cond_principale, NULL));

    pthread_mutex_t mutex;
    TCHK(pthread_mutex_init(&mutex, NULL));

    //initialisation
    for (int i = 0; i < nb_threads; ++i) {
        arg[i].num_thread = i;
        arg[i].nb_thread = nb_threads;
        arg[i].delai_max = delai_max;
        arg[i].cond_inter_thread = &cond_inter_thread;
        arg[i].cond_principale = &cond_principale;
        arg[i].nb_thread_pret = &nb_thread_pret;
        arg[i].mutex = &mutex;
        arg[i].to_print = to_print;
        arg[i].print_iter = &print_iter;
        arg[i].stop = &stop;
    }

    printf("début\n");

    for (int i = 0; i < nb_threads; ++i) {
        TCHK(pthread_create(&tid[i], NULL, fonction, &arg[i]));
    }


    // synchronisation de départ
    pthread_mutex_lock(&mutex);
    while(nb_thread_pret < 2*nb_threads){
        pthread_cond_wait(&cond_principale, &mutex);
    }
    nb_thread_pret = 0; // remise à zéro du nombre de thread prêt pour la boucle principale
    pthread_mutex_unlock(&mutex);

    for (int i = 0; i < taille_cote + 2; ++i) {
        if (i == 0){ // cas particulier pour la première ligne
            for (int j = 0; j < 3; j++){
                pthread_mutex_lock(&mutex);
                if (j == 0){
                    strncpy(to_print, TL_CORNER, MAX_SIZE);
                    print_iter = 1;
                }else if (j == 2){
                    strncpy(to_print, TR_CORNER, MAX_SIZE);
                    print_iter = 1;
                }else{
                    strncpy(to_print, H_BARRE, MAX_SIZE);
                    print_iter = taille_cote;
                }
                pthread_mutex_unlock(&mutex);
                pthread_cond_broadcast(&cond_inter_thread);

                pthread_mutex_lock(&mutex);
                while (print_iter > 0){
                    pthread_cond_wait(&cond_principale, &mutex);
                }
                pthread_mutex_unlock(&mutex);
            }
        }else if (i == taille_cote + 1){ // cas particulier pour la dernière ligne
            for (int j = 0; j < 3; j++){
                pthread_mutex_lock(&mutex);
                if (j == 0){
                    strncpy(to_print, BL_CORNER, MAX_SIZE);
                    print_iter = 1;
                }else if (j == 2){
                    strncpy(to_print, BR_CORNER, MAX_SIZE);
                    print_iter = 1;
                }else{
                    strncpy(to_print, H_BARRE, MAX_SIZE);
                    print_iter = taille_cote;
                }
                pthread_mutex_unlock(&mutex);
                if (print_iter == 1){
                    pthread_cond_signal(&cond_inter_thread);
                } else{
                    pthread_cond_broadcast(&cond_inter_thread);
                }

                pthread_mutex_lock(&mutex);
                while (print_iter > 0){
                    pthread_cond_wait(&cond_principale, &mutex);
                }
                pthread_mutex_unlock(&mutex);
            }
        }else{
            for (int j = 0; j < 3; j++){
                pthread_mutex_lock(&mutex);
                if (j == 0 || j == 2){
                    strncpy(to_print, V_BARRE, MAX_SIZE);
                    print_iter = 1;
                }else{
                    strncpy(to_print, SPACE, MAX_SIZE);
                    print_iter = taille_cote;
                }
                pthread_mutex_unlock(&mutex);
                if (print_iter == 1){
                    pthread_cond_signal(&cond_inter_thread);
                } else{
                    pthread_cond_broadcast(&cond_inter_thread);
                }

                pthread_mutex_lock(&mutex);
                while (print_iter > 0){
                    pthread_cond_wait(&cond_principale, &mutex);
                }
                pthread_mutex_unlock(&mutex);
            }
        }

        pthread_mutex_lock(&mutex);
        strncpy(to_print, NEWLINE, MAX_SIZE);
        print_iter = 1;
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond_inter_thread);

        pthread_mutex_lock(&mutex);
        while (print_iter > 0) {
            pthread_cond_wait(&cond_principale, &mutex);
        }
        if (i == taille_cote + 1){
            stop = TRUE;
            pthread_cond_broadcast(&cond_inter_thread);
        }
        pthread_mutex_unlock(&mutex);

    }

    for (int i = 0; i < nb_threads; ++i) {
        TCHK(pthread_join(tid[i], NULL));
    }

    TCHK(pthread_cond_destroy(&cond_inter_thread));
    TCHK(pthread_cond_destroy(&cond_principale));
    TCHK(pthread_mutex_destroy(&mutex));

    free(tid);
    free(to_print);
    free(arg);

    return 0;
}