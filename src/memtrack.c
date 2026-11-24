#include "memtrack.h"

#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "psapi.lib")   /* GetProcessMemoryInfo, em SDKs antigos */
#endif
#else
#include <sys/resource.h>
#endif

/*
 * Cada bloco leva um cabeçalho escondido com o tamanho pedido, para que
 * mt_free saiba quanto descontar. A union com max_align_t mantém o
 * alinhamento que malloc garantiria.
 *
 * O cabeçalho em si NÃO entra na conta: medimos o que a estrutura pede,
 * não o overhead do alocador, para não distorcer a comparação.
 */
typedef union {
    max_align_t alinhamento;
    size_t tamanho;
} Cabecalho;

static size_t atual = 0;
static size_t pico = 0;

static void *registra(Cabecalho *h, size_t tamanho)
{
    h->tamanho = tamanho;
    atual += tamanho;
    if (atual > pico)
        pico = atual;
    return h + 1;
}

void *mt_malloc(size_t size)
{
    if (size > SIZE_MAX - sizeof(Cabecalho))
        return NULL;
    Cabecalho *h = malloc(sizeof(Cabecalho) + size);
    return h ? registra(h, size) : NULL;
}

void *mt_calloc(size_t n, size_t size)
{
    if (size != 0 && n > SIZE_MAX / size)
        return NULL;
    size_t total = n * size;
    if (total > SIZE_MAX - sizeof(Cabecalho))
        return NULL;
    Cabecalho *h = calloc(1, sizeof(Cabecalho) + total);
    return h ? registra(h, total) : NULL;
}

void *mt_realloc(void *p, size_t size)
{
    if (p == NULL)
        return mt_malloc(size);
    if (size > SIZE_MAX - sizeof(Cabecalho))
        return NULL;

    Cabecalho *h = (Cabecalho *)p - 1;
    size_t antigo = h->tamanho;
    Cabecalho *novo = realloc(h, sizeof(Cabecalho) + size);
    if (novo == NULL)
        return NULL;          /* bloco original continua válido e contado */
    atual -= antigo;
    return registra(novo, size);
}

void mt_free(void *p)
{
    if (p == NULL)
        return;
    Cabecalho *h = (Cabecalho *)p - 1;
    atual -= h->tamanho;
    free(h);
}

size_t mt_current_bytes(void) { return atual; }
size_t mt_peak_bytes(void)    { return pico; }
void   mt_reset_peak(void)    { pico = atual; }

/* Pico de memória residente do processo, em KB, segundo o sistema. */
long mt_peak_rss_kb(void)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS uso;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &uso, sizeof uso))
        return -1;
    return (long)(uso.PeakWorkingSetSize / 1024);
#else
    struct rusage uso;
    if (getrusage(RUSAGE_SELF, &uso) != 0)
        return -1;
#ifdef __APPLE__
    return (long)(uso.ru_maxrss / 1024);   /* no macOS, vem em bytes */
#else
    return uso.ru_maxrss;                  /* no Linux, já vem em KB */
#endif
#endif
}
