#include "dijkstra.h"
#include "memtrack.h"

#include <string.h>

/*
 * Heap binário mínimo indexado, autoral. Guarda vértices, ordenados por
 * (dist[v], v). pos[v] diz onde v está no vetor do heap, ou se ainda não
 * entrou (FORA) ou já saiu (FECHADO). Cada vértice entra uma vez só, então
 * V posições bastam, e diminuir a distância de quem já está no heap é subir
 * ele a partir de pos[v].
 */
#define FORA     SIZE_MAX          /* nunca entrou no heap */
#define FECHADO  (SIZE_MAX - 1)    /* saiu do heap: distância definitiva */

typedef struct {
    size_t *itens;        /* itens[0..tam) formam o heap */
    size_t *pos;          /* posição de cada vértice em itens, FORA ou FECHADO */
    size_t tam;
    const double *dist;   /* a prioridade: as distâncias do resultado */
} Heap;

static int heap_criar(Heap *h, size_t n, const double *dist)
{
    size_t cap = n ? n : 1;
    h->itens = mt_malloc(cap * sizeof *h->itens);
    h->pos = mt_malloc(cap * sizeof *h->pos);
    h->tam = 0;
    h->dist = dist;
    if (!h->itens || !h->pos)
        return 0;
    for (size_t v = 0; v < n; v++)
        h->pos[v] = FORA;
    return 1;
}

static void heap_liberar(Heap *h)
{
    mt_free(h->itens);
    mt_free(h->pos);
}

/* a sai antes de b: menor distância e, no empate, menor índice */
static int antes(const Heap *h, size_t a, size_t b)
{
    if (h->dist[a] != h->dist[b])
        return h->dist[a] < h->dist[b];
    return a < b;
}

static void colocar(Heap *h, size_t i, size_t v)
{
    h->itens[i] = v;
    h->pos[v] = i;
}

static void subir(Heap *h, size_t i)
{
    size_t v = h->itens[i];
    while (i > 0) {
        size_t pai = (i - 1) / 2;
        if (!antes(h, v, h->itens[pai]))
            break;
        colocar(h, i, h->itens[pai]);
        i = pai;
    }
    colocar(h, i, v);
}

static void descer(Heap *h, size_t i)
{
    size_t v = h->itens[i];
    for (;;) {
        size_t menor = 2 * i + 1;
        if (menor >= h->tam)
            break;
        if (menor + 1 < h->tam && antes(h, h->itens[menor + 1], h->itens[menor]))
            menor++;
        if (!antes(h, h->itens[menor], v))
            break;
        colocar(h, i, h->itens[menor]);
        i = menor;
    }
    colocar(h, i, v);
}

/* Insere v, ou sobe v se ele já está no heap e a distância diminuiu. */
static void heap_atualizar(Heap *h, size_t v)
{
    if (h->pos[v] == FORA) {
        colocar(h, h->tam, v);
        h->tam++;
    }
    subir(h, h->pos[v]);
}

static size_t heap_remover(Heap *h)
{
    size_t v = h->itens[0];
    h->pos[v] = FECHADO;
    if (--h->tam > 0) {
        colocar(h, 0, h->itens[h->tam]);
        descer(h, 0);
    }
    return v;
}

void dijkstra_liberar(DijkstraResultado *r)
{
    mt_free(r->dist);
    mt_free(r->pred);
    memset(r, 0, sizeof *r);
}

DijkstraStatus dijkstra_executar(const Graph *g, size_t origem, DijkstraResultado *r)
{
    size_t n = graph_num_vertices(g);
    memset(r, 0, sizeof *r);
    if (origem >= n)
        return DIJKSTRA_ERRO_ORIGEM;

    size_t cap = n ? n : 1;
    r->n = n;
    r->origem = origem;
    r->dist = mt_malloc(cap * sizeof *r->dist);
    r->pred = mt_malloc(cap * sizeof *r->pred);
    Heap h;
    if (!heap_criar(&h, n, r->dist) || !r->dist || !r->pred) {
        heap_liberar(&h);
        dijkstra_liberar(r);
        return DIJKSTRA_ERRO_MEMORIA;
    }
    for (size_t v = 0; v < n; v++) {
        r->dist[v] = DIJKSTRA_INF;
        r->pred[v] = DIJKSTRA_NENHUM;
    }

    r->dist[origem] = 0.0;
    heap_atualizar(&h, origem);
    while (h.tam > 0) {
        size_t u = heap_remover(&h);
        r->alcancados++;

        GraphIter it;
        size_t w;
        float peso;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next_weighted(&it, &w, &peso)) {
            if (peso < 0.0f) {
                heap_liberar(&h);
                dijkstra_liberar(r);
                return DIJKSTRA_ERRO_PESO_NEGATIVO;
            }
            /* com pesos não negativos, um vértice fechado nunca melhora: o
             * teste só poupa a conta */
            if (h.pos[w] == FECHADO)
                continue;
            double nova = r->dist[u] + (double)peso;
            if (nova < r->dist[w]) {   /* estritamente menor: no empate, fica o primeiro */
                r->dist[w] = nova;
                r->pred[w] = u;
                heap_atualizar(&h, w);
            }
        }
    }
    heap_liberar(&h);
    return DIJKSTRA_OK;
}

size_t dijkstra_caminho(const DijkstraResultado *r, size_t destino, size_t *caminho, size_t max)
{
    if (destino >= r->n || r->dist[destino] == DIJKSTRA_INF)
        return 0;
    size_t tam = 1;
    for (size_t v = destino; r->pred[v] != DIJKSTRA_NENHUM; v = r->pred[v])
        tam++;
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
