/*
 * Cronômetro monotônico do RF03, portátil.
 *
 * Windows: QueryPerformanceCounter, em ticks de QueryPerformanceFrequency.
 * Linux e macOS: clock_gettime(CLOCK_MONOTONIC). Com -std=c11, ela só é
 * declarada com _POSIX_C_SOURCE, que o Makefile define; o #ifndef cobre quem
 * compila este arquivo à parte.
 *
 * O Timer é alocado com malloc, e não pelo memtrack, de propósito: ele é
 * criado depois do mt_reset_peak, e o próprio cronômetro não pode entrar na
 * memória medida do algoritmo.
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 199309L
#endif

#include "timer.h"

#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

struct Timer {
#ifdef _WIN32
    LARGE_INTEGER inicio;
#else
    struct timespec inicio;
#endif
};

static void marcar(struct Timer *t)
{
#ifdef _WIN32
    QueryPerformanceCounter(&t->inicio);
#else
    clock_gettime(CLOCK_MONOTONIC, &t->inicio);
#endif
}

/* Milissegundos desde a marcação, calculados pela diferença em unidades
 * inteiras do relógio antes de converter, para não perder precisão. */
static double decorrido_ms(const struct Timer *t)
{
#ifdef _WIN32
    LARGE_INTEGER agora, freq;
    QueryPerformanceCounter(&agora);
    QueryPerformanceFrequency(&freq);
    return (double)(agora.QuadPart - t->inicio.QuadPart) * 1000.0 / (double)freq.QuadPart;
#else
    struct timespec agora;
    clock_gettime(CLOCK_MONOTONIC, &agora);
    long long seg = (long long)agora.tv_sec - (long long)t->inicio.tv_sec;
    long long ns = (long long)agora.tv_nsec - (long long)t->inicio.tv_nsec;
    return (double)seg * 1000.0 + (double)ns / 1000000.0;
#endif
}

/* Aloca e inicia um novo cronômetro */
Timer *timer_criar(void)
{
    Timer *t = malloc(sizeof *t);
    if (t)
        marcar(t);
    return t;
}

/* Dispara a marcação inicial (útil para reaproveitar o mesmo timer) */
void timer_iniciar(Timer *t)
{
    if (t)
        marcar(t);
}

/* Milissegundos desde a última marcação; não reinicia a contagem */
double timer_parar_ms(Timer *t)
{
    return t ? decorrido_ms(t) : 0.0;
}

/* Libera a memória alocada pelo timer */
void timer_destruir(Timer *t)
{
    free(t);
}
