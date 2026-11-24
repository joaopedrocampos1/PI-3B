/*
 * Testes da medição de memória (src/memtrack.c), que mede a memória do RF03.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_memtrack && ./bin/test_memtrack
 */
#include "memtrack.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int falhas = 0;
static int verificacoes = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        verificacoes++;                                                      \
        if (!(cond)) {                                                       \
            falhas++;                                                        \
            fprintf(stderr, "  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

/* O contador soma exatamente os bytes pedidos e os devolve na liberação. */
static void conta_os_bytes_pedidos(void)
{
    size_t antes = mt_current_bytes();
    char *a = mt_malloc(1000);
    CHECK(a && mt_current_bytes() == antes + 1000);

    int *b = mt_calloc(10, sizeof *b);
    CHECK(b && mt_current_bytes() == antes + 1000 + 10 * sizeof *b);
    int zerado = 1;
    for (int i = 0; b && i < 10; i++)
        zerado &= b[i] == 0;
    CHECK(zerado);

    a = mt_realloc(a, 3000);
    CHECK(a && mt_current_bytes() == antes + 3000 + 10 * sizeof *b);

    mt_free(a);
    mt_free(b);
    mt_free(NULL);
    CHECK(mt_current_bytes() == antes);
}

/* O pico guarda o maior valor desde o último reset, que o recomeça do atual. */
static void pico_e_reset(void)
{
    mt_reset_peak();
    size_t base = mt_current_bytes();
    void *p = mt_malloc(5000);
    mt_free(p);
    CHECK(mt_peak_bytes() == base + 5000);
    CHECK(mt_current_bytes() == base);

    mt_reset_peak();
    CHECK(mt_peak_bytes() == base);
}

/* Pedidos impossíveis falham sem alterar o contador. */
static void pedidos_impossiveis(void)
{
    size_t antes = mt_current_bytes();
    CHECK(mt_malloc(SIZE_MAX) == NULL);
    CHECK(mt_calloc(SIZE_MAX / 2, 4) == NULL);
    CHECK(mt_current_bytes() == antes);
}

/* Pico de memória residente do processo, em KB. Depois de tocar 32 MB, ele
 * precisa estar entre 32 MB e 4 GB, em KB. O teto pega erro de unidade:
 * em bytes, os mesmos 32 MB passariam de 33 milhões. */
static void pico_residente_em_kb(void)
{
    const size_t tam = (size_t)32 * 1024 * 1024;
    char *p = mt_malloc(tam);
    CHECK(p != NULL);
    if (p)
        memset(p, 1, tam);   /* tocar as páginas para que fiquem residentes */
    long kb = mt_peak_rss_kb();
    CHECK(kb >= 32L * 1024);
    CHECK(kb < 4L * 1024 * 1024);
    mt_free(p);
}

int main(void)
{
    conta_os_bytes_pedidos();
    pico_e_reset();
    pedidos_impossiveis();
    pico_residente_em_kb();

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
