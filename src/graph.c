#include "graph_internal.h"
#include "memtrack.h"

#include <stdlib.h>

/* Representações disponíveis, indexadas por GraphRep. As tabelas de operações
 * são declaradas em graph_internal.h. */
static const GraphOps *const REPRESENTACOES[] = {
    [GRAPH_LIST] = &GRAPH_LIST_OPS,
    [GRAPH_MATRIX] = &GRAPH_MATRIX_OPS,
};

static int sem_sentido(GraphView view)
{
    return view != GRAPH_DIRECTED;
}

GraphStatus graph_create(size_t n, GraphRep rep, GraphView view, Graph **out)
{
    *out = NULL;
    const GraphOps *ops = REPRESENTACOES[rep];
    if (!ops)
        return GRAPH_ERR_UNAVAILABLE;

    Graph *g = mt_malloc(sizeof *g);
    if (!g)
        return GRAPH_ERR_MEMORY;
    g->ops = ops;
    g->rep = rep;
    g->view = view;
    g->n = n;
    g->m = 0;
    g->data = ops->create(n);
    if (!g->data) {
        mt_free(g);
        return GRAPH_ERR_MEMORY;
    }
    *out = g;
    return GRAPH_OK;
}

void graph_destroy(Graph *g)
{
    if (!g)
        return;
    g->ops->destroy(g->data, g->n);
    mt_free(g);
}

GraphStatus graph_add_edge(Graph *g, size_t u, size_t v)
{
    if (u == v)
        return GRAPH_OK;

    int r = g->ops->add(g->data, g->n, u, v);
    if (r < 0)
        return GRAPH_ERR_MEMORY;
    if (sem_sentido(g->view) && g->ops->add(g->data, g->n, v, u) < 0)
        return GRAPH_ERR_MEMORY;
    if (r == 1)
        g->m++;
    return GRAPH_OK;
}

/* A edge list vem ordenada por (origem, destino), então dá para procurar
 * a aresta de volta por busca binária. */
static int compara_aresta(const void *a, const void *b)
{
    const Aresta *x = a, *y = b;
    if (x->origem != y->origem)
        return x->origem < y->origem ? -1 : 1;
    if (x->destino != y->destino)
        return x->destino < y->destino ? -1 : 1;
    return 0;
}

static int tem_volta(const EdgeList *el, const Aresta *a)
{
    Aresta volta = {a->destino, a->origem};
    return bsearch(&volta, el->arestas, el->n, sizeof *el->arestas, compara_aresta) != NULL;
}

GraphStatus graph_build(const EdgeList *el, IdMap *ids, GraphRep rep, GraphView view,
                        Graph **out)
{
    *out = NULL;
    size_t u, v;
    for (size_t e = 0; e < el->n; e++)
        if (!idmap_inserir(ids, el->arestas[e].origem, &u) ||
            !idmap_inserir(ids, el->arestas[e].destino, &v))
            return GRAPH_ERR_MEMORY;

    Graph *g;
    GraphStatus st = graph_create(ids->n, rep, view, &g);
    if (st != GRAPH_OK)
        return st;

    for (size_t e = 0; e < el->n; e++) {
        const Aresta *a = &el->arestas[e];
        /* cada par mútuo aparece duas vezes na lista; basta inserir uma */
        if (view == GRAPH_RECIPROCAL && (a->origem > a->destino || !tem_volta(el, a)))
            continue;
        idmap_buscar(ids, a->origem, &u);
        idmap_buscar(ids, a->destino, &v);
        if (graph_add_edge(g, u, v) != GRAPH_OK) {
            graph_destroy(g);
            return GRAPH_ERR_MEMORY;
        }
    }
    *out = g;
    return GRAPH_OK;
}

int graph_has_edge(const Graph *g, size_t u, size_t v)
{
    return g->ops->has(g->data, g->n, u, v);
}

size_t graph_degree(const Graph *g, size_t v)
{
    return g->ops->degree(g->data, g->n, v);
}

size_t    graph_num_vertices(const Graph *g) { return g->n; }
size_t    graph_num_edges(const Graph *g)    { return g->m; }
GraphRep  graph_rep(const Graph *g)          { return g->rep; }
GraphView graph_view(const Graph *g)         { return g->view; }

void graph_neighbors_begin(const Graph *g, size_t v, GraphIter *it)
{
    it->g = g;
    it->v = v;
    it->pos = 0;
}

int graph_neighbors_next(GraphIter *it, size_t *w)
{
    const Graph *g = it->g;
    return g->ops->next(g->data, g->n, it->v, &it->pos, w);
}

const char *graph_rep_name(GraphRep rep)
{
    return rep == GRAPH_MATRIX ? "matriz" : "lista";
}

const char *graph_view_name(GraphView view)
{
    switch (view) {
    case GRAPH_SYMMETRIC:  return "simetrizada";
    case GRAPH_RECIPROCAL: return "reciproca";
    default:               return "direcionada";
    }
}
