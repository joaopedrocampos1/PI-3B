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
 *
 * Com peso (#64): ao lado do vetor de vizinhos de cada vértice, um vetor de
 * pesos paralelo, com a mesma capacidade (peso[v][i] é o peso da aresta até
 * viz[i]). A lista sem peso não aloca nada disso.
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

/* Insere v na lista l, mantendo a ordem. `pesos` é o vetor de pesos
 * paralelo (NULL na lista sem peso), e `peso` o valor a gravar nele: numa
 * aresta que já existe, só o peso é trocado. Retorna 1 se inseriu, 0 se
 * já existia, -1 sem memória. */
static int inserir(Lista *l, float **pesos, size_t v, float peso)
{
    /* a edge list chega ordenada, então o caso comum é anexar no fim */
    size_t p = (l->grau && l->viz[l->grau - 1] < v) ? l->grau : posicao(l, v);
    if (p < l->grau && l->viz[p] == v) {
        if (pesos)
            (*pesos)[p] = peso;
        return 0;
    }

    if (l->grau == l->cap) {
        size_t nova = l->cap ? l->cap * 2 : CAP_INICIAL;
        size_t *viz = mt_realloc(l->viz, nova * sizeof *viz);
        if (!viz)
            return -1;
        l->viz = viz;
        if (pesos) {
            /* se faltar memória aqui, viz fica maior que cap, o que é
             * inofensivo: cap só cresce quando os dois vetores crescem */
            float *pv = mt_realloc(*pesos, nova * sizeof *pv);
            if (!pv)
                return -1;
            *pesos = pv;
        }
        l->cap = nova;
    }
    memmove(l->viz + p + 1, l->viz + p, (l->grau - p) * sizeof *l->viz);
    l->viz[p] = v;
    if (pesos) {
        memmove(*pesos + p + 1, *pesos + p, (l->grau - p) * sizeof **pesos);
        (*pesos)[p] = peso;
    }
    l->grau++;
    return 1;
}

static int lista_add(void *data, size_t n, size_t u, size_t v)
{
    (void)n;
    return inserir((Lista *)data + u, NULL, v, 0.0f);
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
    NULL,
    NULL,
    NULL,
};

/* ---- Lista com peso (#64) ---- */

typedef struct {
    Lista *listas;   /* as mesmas listas da versão sem peso */
    float **pesos;   /* pesos[v]: vetor paralelo a listas[v].viz */
} ListaComPeso;

static void *lista_peso_create(size_t n)
{
    ListaComPeso *lp = mt_malloc(sizeof *lp);
    if (!lp)
        return NULL;
    lp->listas = lista_create(n);
    lp->pesos = mt_calloc(n ? n : 1, sizeof *lp->pesos);
    if (!lp->listas || !lp->pesos) {
        mt_free(lp->listas);
        mt_free(lp->pesos);
        mt_free(lp);
        return NULL;
    }
    return lp;
}

static void lista_peso_destroy(void *data, size_t n)
{
    ListaComPeso *lp = data;
    for (size_t v = 0; v < n; v++)
        mt_free(lp->pesos[v]);
    mt_free(lp->pesos);
    lista_destroy(lp->listas, n);
    mt_free(lp);
}

static int lista_peso_add_weighted(void *data, size_t n, size_t u, size_t v, float peso)
{
    (void)n;
    ListaComPeso *lp = data;
    return inserir(lp->listas + u, &lp->pesos[u], v, peso);
}

static int lista_peso_add(void *data, size_t n, size_t u, size_t v)
{
    return lista_peso_add_weighted(data, n, u, v, 1.0f);
}

static int lista_peso_has(const void *data, size_t n, size_t u, size_t v)
{
    return lista_has(((const ListaComPeso *)data)->listas, n, u, v);
}

static size_t lista_peso_degree(const void *data, size_t n, size_t v)
{
    return lista_degree(((const ListaComPeso *)data)->listas, n, v);
}

static int lista_peso_next(const void *data, size_t n, size_t v, size_t *pos, size_t *w)
{
    return lista_next(((const ListaComPeso *)data)->listas, n, v, pos, w);
}

static float lista_peso_weight(const void *data, size_t n, size_t u, size_t v)
{
    (void)n;
    const ListaComPeso *lp = data;
    return lp->pesos[u][posicao(lp->listas + u, v)];
}

static int lista_peso_next_weighted(const void *data, size_t n, size_t v, size_t *pos,
                                    size_t *w, float *peso)
{
    const ListaComPeso *lp = data;
    size_t i = *pos;
    if (!lista_next(lp->listas, n, v, pos, w))
        return 0;
    *peso = lp->pesos[v][i];
    return 1;
}

const GraphOps GRAPH_LIST_WEIGHTED_OPS = {
    lista_peso_create,
    lista_peso_destroy,
    lista_peso_add,
    lista_peso_has,
    lista_peso_degree,
    lista_peso_next,
    lista_peso_add_weighted,
    lista_peso_weight,
    lista_peso_next_weighted,
};
