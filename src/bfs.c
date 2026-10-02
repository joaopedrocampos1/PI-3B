#include "bfs.h"
#include "memtrack.h"

#include <string.h>

/*
 * Fila autoral em vetor. Cada vértice é enfileirado no máximo uma vez
 * (só quando é visitado pela primeira vez), então V posições bastam e os
 * índices nunca precisam dar a volta.
 */
typedef struct {
    size_t *itens;
    size_t ini, fim;   /* itens[ini..fim) estão na fila */
} Fila;

static int fila_criar(Fila *f, size_t cap)
{
    f->itens = mt_malloc((cap ? cap : 1) * sizeof *f->itens);
    f->ini = f->fim = 0;
    return f->itens != NULL;
}

static void fila_inserir(Fila *f, size_t v) { f->itens[f->fim++] = v; }
static size_t fila_remover(Fila *f)         { return f->itens[f->ini++]; }
static int fila_vazia(const Fila *f)        { return f->ini == f->fim; }
static void fila_liberar(Fila *f)           { mt_free(f->itens); }

void bfs_liberar(BfsResultado *r)
{
    mt_free(r->visitado);
    mt_free(r->dist);
    mt_free(r->pred);
    memset(r, 0, sizeof *r);
}

static BfsStatus preparar(BfsResultado *r, size_t n)
{
    memset(r, 0, sizeof *r);
    size_t cap = n ? n : 1;
    r->n = n;
    r->visitado = mt_calloc(cap, sizeof *r->visitado);
    r->dist = mt_malloc(cap * sizeof *r->dist);
    r->pred = mt_malloc(cap * sizeof *r->pred);
    if (!r->visitado || !r->dist || !r->pred) {
        bfs_liberar(r);
        return BFS_ERRO_MEMORIA;
    }
    for (size_t v = 0; v < n; v++) {
        r->dist[v] = BFS_INF;
        r->pred[v] = BFS_NENHUM;
    }
    return BFS_OK;
}

BfsStatus bfs_executar_multi(const Graph *g, const size_t *origens, size_t k, BfsResultado *r)
{
    size_t n = graph_num_vertices(g);
    memset(r, 0, sizeof *r);
    if (k == 0)
        return BFS_ERRO_ORIGEM;
    for (size_t i = 0; i < k; i++)
        if (origens[i] >= n)
            return BFS_ERRO_ORIGEM;

    BfsStatus st = preparar(r, n);
    if (st != BFS_OK)
        return st;
    Fila fila;
    if (!fila_criar(&fila, n)) {
        bfs_liberar(r);
        return BFS_ERRO_MEMORIA;
    }

    for (size_t i = 0; i < k; i++) {
        size_t s = origens[i];
        if (r->visitado[s])   /* origem repetida */
            continue;
        r->visitado[s] = 1;
        r->dist[s] = 0;
        fila_inserir(&fila, s);
    }

    /* soma em size_t: no pior caso V * (V - 1), cabe com folga em 64 bits */
    size_t soma = 0, contados = 0;
    while (!fila_vazia(&fila)) {
        size_t u = fila_remover(&fila);
        GraphIter it;
        size_t w;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next(&it, &w)) {
            if (r->visitado[w])
                continue;
            r->visitado[w] = 1;
            r->dist[w] = r->dist[u] + 1;
            r->pred[w] = u;
            soma += r->dist[w];
            contados++;
            if (r->dist[w] > r->dist_max)
                r->dist_max = r->dist[w];
            fila_inserir(&fila, w);
        }
    }

    /* todo vértice enfileirado foi alcançado, e vice-versa */
    r->alcancados = fila.fim;
    r->dist_media = contados ? (double)soma / (double)contados : 0.0;
    fila_liberar(&fila);
    return BFS_OK;
}

BfsStatus bfs_executar(const Graph *g, size_t origem, BfsResultado *r)
{
    return bfs_executar_multi(g, &origem, 1, r);
}

size_t bfs_caminho(const BfsResultado *r, size_t destino, size_t *caminho, size_t max)
{
    if (destino >= r->n || !r->visitado[destino])
        return 0;
    size_t tam = r->dist[destino] + 1;
    if (tam > max)
        return tam;

    /* os predecessores levam do destino à origem: grava de trás para frente */
    size_t v = destino;
    for (size_t i = tam; i-- > 0;) {
        caminho[i] = v;
        v = r->pred[v];
    }
    return tam;
}
