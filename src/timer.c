/* Define a macro necessária para habilitar o clock_gettime no padrão C11 */
#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <stdlib.h>

/* Estrutura interna para encapsular a marcação de tempo */
typedef struct {
    struct timespec start_time;
    struct timespec end_time;
} Timer;

/* Aloca e inicia um novo cronômetro */
Timer* timer_criar(void) {
    Timer *t = malloc(sizeof(Timer));
    if (!t) return NULL;
    clock_gettime(CLOCK_MONOTONIC, &(t->start_time));
    return t;
}

/* Dispara a marcação inicial (útil para reaproveitar o mesmo timer) */
void timer_iniciar(Timer *t) {
    if (t) {
        clock_gettime(CLOCK_MONOTONIC, &(t->start_time));
    }
}

/* Para o cronômetro e retorna a diferença exata em milissegundos (ms) */
double timer_parar_ms(Timer *t) {
    if (!t) return 0.0;
    
    clock_gettime(CLOCK_MONOTONIC, &(t->end_time));
    
    double start_ms = (t->start_time.tv_sec * 1000.0) + (t->start_time.tv_nsec / 1000000.0);
    double end_ms = (t->end_time.tv_sec * 1000.0) + (t->end_time.tv_nsec / 1000000.0);
    
    return end_ms - start_ms;
}

/* Libera a memória alocada pelo timer */
void timer_destruir(Timer *t) {
    if (t) {
        free(t);
    }
}
