#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <semaphore.h>  // necessário para sem_t

#define tam_alfabeto 26  // Limite de A-Z

// buffer e tamanho dele
char *buffer;
int tam_buffer;
// começa apontando na posição 0
int pos_buffer = 0;

int exec_count[tam_alfabeto];
// semáforo binário para controlar acesso ao buffer
sem_t semaforo;

typedef struct {
    int id;
    char symbol;
} ThreadArg;

// função para delay em microssegundos (usando busy-wait com gettimeofday)
void delay_us(unsigned long delay)
{
    struct timeval time1, time2;
    long long timel1, timel2;

    gettimeofday(&time1, 0);

    timel1 = time1.tv_sec * 1000000LL;
    timel1 += time1.tv_usec;

    do {
        gettimeofday(&time2, 0);
        timel2 = time2.tv_sec * 1000000LL;
        timel2 += time2.tv_usec;
    } while (timel2 - timel1 < delay);
}

void* thread_func(void* arg) {
    ThreadArg *targ = (ThreadArg*) arg;

    while (1) {
        // delay de 1ms
        delay_us(1000);

        // entra na seção crítica usando o semáforo
        sem_wait(&semaforo);
        //verifica se acabou o buffer
        if (pos_buffer >= tam_buffer) {
            sem_post(&semaforo);
            break;
        }
        //senao grava a letra correspondente no buffer, atualiza a posicao do ponteiro e aumenta o contador daquela letra 
        buffer[pos_buffer++] = targ->symbol;
        exec_count[targ->id]++;
        // libera a seção crítica
        sem_post(&semaforo);
    }

    return NULL;
}

int main(int argc, char *argv[]) {  
    
    // $ ./thread_runner <numero_de_threads> <tamanho_do_buffer_global_em_kb> <politica> <prioridade>
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <num_threads> <tam_buffer> <politica> <prioridade>\n", argv[0]);
        return 1;
    }

    // pega os parâmetros
    int num_threads = atoi(argv[1]);
    int tam_buffer_arg = atoi(argv[2]);
    char *politica_str = argv[3];
    int prioridade = atoi(argv[4]);

    // se passar o limite de threads
    if (num_threads > tam_alfabeto) {
        fprintf(stderr, "Máximo de %d threads suportado. Tente novamente\n", tam_alfabeto);
        return 1;
    }

    // tam do buffer em bytes 
    tam_buffer = tam_buffer_arg * 1024;
    // aloca o buffer
    buffer = malloc(tam_buffer);
    if (!buffer) {
        perror("Erro ao alocar buffer");
        return 1;
    }

    // inicializa o semáforo binário (1 = recurso livre)
    sem_init(&semaforo, 0, 1);

    // cria os vetores de threads no tamanho máximo possível
    pthread_t threads[tam_alfabeto];
    ThreadArg args[tam_alfabeto];

    // escolher a política de acordo com a entrada
    int politica;
    if (strcmp(politica_str, "SCHED_FIFO") == 0) politica = SCHED_FIFO;
    else if (strcmp(politica_str, "SCHED_RR") == 0) politica = SCHED_RR;
    else if (strcmp(politica_str, "SCHED_OTHER") == 0) politica = SCHED_OTHER;
    else if (strcmp(politica_str, "SCHED_BATCH") == 0) politica = SCHED_BATCH;
    else if (strcmp(politica_str, "SCHED_IDLE") == 0) politica = SCHED_IDLE;
    else {
        fprintf(stderr, "Política desconhecida. Tente novamente.\n");
        return 1;
    }

    // pra cada thread
    for (int i = 0; i < num_threads; i++) {
        
        // endereça qual é o id e o caractere dele
        args[i].id = i;
        args[i].symbol = 'A' + i;

        // inicia a thread
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        struct sched_param param;
        param.sched_priority = prioridade;

        // atribui a política escolhida
        pthread_attr_setschedpolicy(&attr, politica);

        // salva a prioridade
        pthread_attr_setschedparam(&attr, &param);

        // faz com que force a utilização da política independente do pai
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        
        // caso de erro
        if (pthread_create(&threads[i], &attr, thread_func, &args[i]) != 0) {
            perror("Erro ao criar thread. Tente novamente.");
            return 1;
        }

        // libera os recursos pra criação da thread
        pthread_attr_destroy(&attr);
    }

    // espera todas elas terminarem
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // imprime o buffer resultante
    fwrite(buffer, 1, tam_buffer, stdout);
    printf("\n\n");


    // imprime as letras utilizadas
    for (int i = 0; i < num_threads; i++) {
        printf("%c", 'A' + i);
    }
    printf("\n");

    // imprime quantas vezes cada thread escreveu
    for (int i = 0; i < num_threads; i++) {
        printf("%c = %d\n", 'A' + i, exec_count[i]);
    }

    //libera o buffer
    free(buffer);

    // destrói o semáforo
    sem_destroy(&semaforo);
    return 0;
}
