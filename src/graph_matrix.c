#include "graph_internal.h"
#include "memtrack.h"

#include <stdint.h>

/*
 * Matriz de adjacência (#11).
 *
 * Matriz de bits: cada linha é um vetor de palavras de 64 bits, e o bit
 * (u, v) diz se existe a aresta u -> v. Com um bit por célula, em vez de um
 * byte, a matriz ocupa 8 vezes menos: no grafo completo (81 mil vértices)
 * são cerca de 830 MB em vez de 6,6 GB.
 *
 * O grau de cada vértice fica guardado à parte, para graph_degree ser O(1)
 * como na lista. Custa O(V), desprezível perto dos O(V²) da matriz.
 *
 * Memória: O(V²) bits, alocada inteira em graph_create, mesmo sem arestas.
 */

#define BITS_PALAVRA 64

typedef struct {
    size_t palavras;   /* palavras de 64 bits por linha */
    size_t *grau;
    uint64_t *bits;    /* n linhas de `palavras` palavras, contíguas */
} Matriz;

static uint64_t *linha(const Matriz *m, size_t u)
{
    return m->bits + u * m->palavras;
}

/* Índice do bit 1 menos significativo. Exige x != 0. */
static size_t primeiro_bit(uint64_t x)
{
#if defined(__GNUC__)
    return (size_t)__builtin_ctzll(x);
#else
    size_t i = 0;
    while (!(x & 1)) {
        x >>= 1;
        i++;
    }
    return i;
#endif
}

static void *matriz_create(size_t n)
{
    Matriz *m = mt_malloc(sizeof *m);
    if (!m)
        return NULL;
    m->palavras = (n + BITS_PALAVRA - 1) / BITS_PALAVRA;

    /* n == 0: aloca 1 elemento, só para não depender de calloc(0) */
    size_t celulas = n ? n : 1;
    size_t palavras = m->palavras ? m->palavras : 1;
    if (celulas > SIZE_MAX / palavras) {   /* n * palavras estouraria size_t */
        mt_free(m);
        return NULL;
    }
    m->grau = mt_calloc(celulas, sizeof *m->grau);
    m->bits = mt_calloc(celulas * palavras, sizeof *m->bits);
    if (!m->grau || !m->bits) {
        mt_free(m->grau);
        mt_free(m->bits);
        mt_free(m);
        return NULL;
    }
    return m;
}

static void matriz_destroy(void *data, size_t n)
{
    (void)n;
    Matriz *m = data;
    mt_free(m->grau);
    mt_free(m->bits);
    mt_free(m);
}

static int matriz_add(void *data, size_t n, size_t u, size_t v)
{
    (void)n;
    Matriz *m = data;
    uint64_t *p = &linha(m, u)[v / BITS_PALAVRA];
    uint64_t bit = (uint64_t)1 << (v % BITS_PALAVRA);
    if (*p & bit)
        return 0;
    *p |= bit;
    m->grau[u]++;
    return 1;
}

static int matriz_has(const void *data, size_t n, size_t u, size_t v)
{
    (void)n;
    const Matriz *m = data;
    return (linha(m, u)[v / BITS_PALAVRA] >> (v % BITS_PALAVRA)) & 1;
}

static size_t matriz_degree(const void *data, size_t n, size_t v)
{
    (void)n;
    return ((const Matriz *)data)->grau[v];
}

/* *pos é a próxima coluna a examinar. Palavras inteiras sem nenhum bit são
 * puladas de uma vez, então percorrer os vizinhos custa O(V / 64 + grau). */
static int matriz_next(const void *data, size_t n, size_t v, size_t *pos, size_t *w)
{
    const Matriz *m = data;
    if (*pos >= n)
        return 0;

    const uint64_t *l = linha(m, v);
    size_t k = *pos / BITS_PALAVRA;
    /* descarta as colunas anteriores a *pos na primeira palavra */
    uint64_t palavra = l[k] & (~(uint64_t)0 << (*pos % BITS_PALAVRA));
    while (!palavra) {
        if (++k >= m->palavras) {
            *pos = n;
            return 0;
        }
        palavra = l[k];
    }
    *w = k * BITS_PALAVRA + primeiro_bit(palavra);
    *pos = *w + 1;
    return 1;
}

const GraphOps GRAPH_MATRIX_OPS = {
    matriz_create,
    matriz_destroy,
    matriz_add,
    matriz_has,
    matriz_degree,
    matriz_next,
};
