#include "graph_internal.h"
#include "memtrack.h"

#include <string.h>

/*
 * Lista de adjacência (#10).
 *
 * Cada vértice tem um vetor dinâmico com os índices dos vizinhos, mantido
 * em ordem crescente e sem repetição. A ordem dá graph_has_edge por busca
 * binária, O(log grau), e o percurso de vizinhos na mesma ordem da matriz.
 *
 * Memória: O(V + E). Um vértice sem vizinhos não aloca vetor.
 */

#define CAP_INICIAL 4

typedef struct {
    size_t *viz;
    size_t grau;
    size_t cap;
} Lista;

static void *lista_create(size_t n)
{
    return mt_calloc(n ? n : 1, sizeof(Lista));
}

static void lista_destroy(void *data, size_t n)
{
    Lista *l = data;
    for (size_t v = 0; v < n; v++)
        mt_free(l[v].viz);
    mt_free(l);
}

/* Posição de w em l->viz, ou onde ele deveria entrar. */
static size_t posicao(const Lista *l, size_t w)
{
    size_t ini = 0, fim = l->grau;
    while (ini < fim) {
        size_t meio = ini + (fim - ini) / 2;
        if (l->viz[meio] < w)
            ini = meio + 1;
        else
            fim = meio;
    }
    return ini;
}

static int lista_add(void *data, size_t n, size_t u, size_t v)
{
    (void)n;
    Lista *l = (Lista *)data + u;

    /* a edge list chega ordenada, então o caso comum é anexar no fim */
    size_t p = (l->grau && l->viz[l->grau - 1] < v) ? l->grau : posicao(l, v);
    if (p < l->grau && l->viz[p] == v)
        return 0;

    if (l->grau == l->cap) {
        size_t nova = l->cap ? l->cap * 2 : CAP_INICIAL;
        size_t *viz = mt_realloc(l->viz, nova * sizeof *viz);
        if (!viz)
            return -1;
        l->viz = viz;
        l->cap = nova;
    }
    memmove(l->viz + p + 1, l->viz + p, (l->grau - p) * sizeof *l->viz);
    l->viz[p] = v;
    l->grau++;
    return 1;
}

static int lista_has(const void *data, size_t n, size_t u, size_t v)
{
    (void)n;
    const Lista *l = (const Lista *)data + u;
    size_t p = posicao(l, v);
    return p < l->grau && l->viz[p] == v;
}

static size_t lista_degree(const void *data, size_t n, size_t v)
{
    (void)n;
    return ((const Lista *)data)[v].grau;
}

/* *pos é a posição no vetor de vizinhos. */
static int lista_next(const void *data, size_t n, size_t v, size_t *pos, size_t *w)
{
    (void)n;
    const Lista *l = (const Lista *)data + v;
    if (*pos >= l->grau)
        return 0;
    *w = l->viz[(*pos)++];
    return 1;
}

const GraphOps GRAPH_LIST_OPS = {
    lista_create,
    lista_destroy,
    lista_add,
    lista_has,
    lista_degree,
    lista_next,
};
