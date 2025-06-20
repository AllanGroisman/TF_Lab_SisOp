#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define tam_alfabeto 26  // Limite de A-Z

//buffer e tamanho dele
char *buffer;
int tam_buffer;
//comeca apontando na posicao 0
int pos_buffer = 0;

int exec_count[tam_alfabeto];
pthread_mutex_t mutex;

typedef struct {
    int id;
    char symbol;
} ThreadArg;

void* thread_func(void* arg) {
    ThreadArg *targ = (ThreadArg*) arg;

    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 1000000; // 1ms

    while (1) {
        nanosleep(&delay, NULL);

        pthread_mutex_lock(&mutex);
        if (pos_buffer >= tam_buffer) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        buffer[pos_buffer++] = targ->symbol;
        exec_count[targ->id]++;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {  
    
    // $ ./thread_runner <numero_de_threads> <tamanho_do_buffer_global_em_kb> <politica> <prioridade>
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <num_threads> <tam_buffer> <politica> <prioridade>\n", argv[0]);
        return 1;
    }

    //pega os parametros
    int num_threads = atoi(argv[1]);
    int tam_buffer_arg = atoi(argv[2]);
    char *politica_str = argv[3];
    int prioridade = atoi(argv[4]);

    //se passar o limite de threads
    if (num_threads > tam_alfabeto) {
        fprintf(stderr, "Máximo de %d threads suportado. Tente novamente\n", tam_alfabeto);
        return 1;
    }

    //tam do buffer em bytes 
    tam_buffer = tam_buffer_arg * 1024;
    //aloca o muffer
    buffer = malloc(tam_buffer);
    if (!buffer) {
        perror("Erro ao alocar buffer");
        return 1;
    }

    //cria o mutex
    pthread_mutex_init(&mutex, NULL);

    //cria os vetores de threads no tamanho maximo possivel
    pthread_t threads[tam_alfabeto];
    ThreadArg args[tam_alfabeto];

    //escolher a politica de acorda com a entrada
    int politica;
    if (strcmp(politica_str, "SCHED_FIFO") == 0) politica = SCHED_FIFO;
    else if (strcmp(politica_str, "SCHED_RR") == 0) politica = SCHED_RR;
    else if (strcmp(politica_str, "SCHED_OTHER") == 0) politica = SCHED_OTHER;
    else if (strcmp(politica_str, "SCHED_BATCH") == 0) politica = SCHED_BATCH;
    else if (strcmp(politica_str, "SCHED_IDLE") == 0) politica = SCHED_IDLE;
    else {
        fprintf(stderr, "Política desconhecida.Tente novamente.\n");
        return 1;
    }

    //pra cada thread
    for (int i = 0; i < num_threads; i++) {
        
        //endereça qual é o id e o caracter dele
        args[i].id = i;
        args[i].symbol = 'A' + i;

        //inicia a thread
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        struct sched_param param;
        param.sched_priority = prioridade;

        //atribui a politica escolhida
        pthread_attr_setschedpolicy(&attr, politica);

        //salva a prioridade
        pthread_attr_setschedparam(&attr, &param);

        //faz com que force a utilizacao da politica independente do pai
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        
        //caso de erro
        if (pthread_create(&threads[i], &attr, thread_func, &args[i]) != 0) {
            perror("Erro ao criar thread.Tente novamente.");
            return 1;
        }

        //libera os recursos pra criacao da thread
        pthread_attr_destroy(&attr);
    }

    // comeca todas elas
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    fwrite(buffer, 1, tam_buffer, stdout);
    printf("\n");

    for (int i = 0; i < num_threads; i++) {
        printf("%c", 'A' + i);
    }
    printf("\n");

    for (int i = 0; i < num_threads; i++) {
        printf("%c = %d\n", 'A' + i, exec_count[i]);
    }

    free(buffer);
    pthread_mutex_destroy(&mutex);
    return 0;
}
