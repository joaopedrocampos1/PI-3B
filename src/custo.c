#include "custo.h"
#include "memtrack.h"

#include <string.h>

size_t custo_comuns(const Graph *sim, size_t a, size_t b)
{
    /* percorre o círculo menor e consulta cada vizinho no maior */
    if (graph_degree(sim, a) > graph_degree(sim, b)) {
        size_t t = a;
        a = b;
        b = t;
    }
    size_t c = 0, w;
    GraphIter it;
    graph_neighbors_begin(sim, a, &it);
    while (graph_neighbors_next(&it, &w))
        if (w != b && graph_has_edge(sim, b, w))
            c++;
    return c;
}

/* c da aresta a -> b: os contatos em comum, mais 1 se a volta existe */
static size_t contatos(const Graph *dir, const Graph *sim, size_t a, size_t b)
{
    return custo_comuns(sim, a, b) + (graph_has_edge(dir, b, a) ? 1 : 0);
}

float custo_aresta(const Graph *dir, const Graph *sim, size_t a, size_t b)
{
    return 1.0f / (1.0f + (float)contatos(dir, sim, a, b));
}

CustoStatus custo_construir(const Graph *dir, const Graph *sim, GraphRep rep,
                            CustoSentido sentido, Graph **out, CustoResumo *resumo)
{
    *out = NULL;
    if (resumo)
        memset(resumo, 0, sizeof *resumo);
    size_t n = graph_num_vertices(dir);
    if (graph_view(dir) != GRAPH_DIRECTED || graph_view(sim) != GRAPH_SYMMETRIC ||
        graph_num_vertices(sim) != n)
        return CUSTO_ERRO_VISAO;

    Graph *g;
    if (graph_create_weighted(n, rep, GRAPH_DIRECTED, &g) != GRAPH_OK)
        return CUSTO_ERRO_MEMORIA;

    /* c nunca passa de n - 1: no máximo n - 2 contatos em comum, mais a volta */
    size_t *hist = NULL;
    if (resumo) {
        hist = mt_calloc(n ? n : 1, sizeof *hist);
        if (!hist) {
            graph_destroy(g);
            return CUSTO_ERRO_MEMORIA;
        }
    }

    size_t arestas = 0, mutuas = 0, maior = 0;
    for (size_t a = 0; a < n; a++) {
        GraphIter it;
        size_t b;
        graph_neighbors_begin(dir, a, &it);
        while (graph_neighbors_next(&it, &b)) {
            int volta = graph_has_edge(dir, b, a);
            size_t c = custo_comuns(sim, a, b) + (volta ? 1 : 0);
            float peso = 1.0f / (1.0f + (float)c);
            GraphStatus st = sentido == CUSTO_SENTIDO_PROPAGACAO
                                 ? graph_add_weighted_edge(g, b, a, peso)
                                 : graph_add_weighted_edge(g, a, b, peso);
            if (st != GRAPH_OK) {
                mt_free(hist);
                graph_destroy(g);
                return CUSTO_ERRO_MEMORIA;
            }
            arestas++;
            mutuas += volta ? 1 : 0;
            if (hist) {
                hist[c]++;
                if (c > maior)
                    maior = c;
            }
        }
    }

    if (resumo) {
        resumo->arestas = arestas;
        resumo->mutuas = mutuas;
        resumo->tam_hist = arestas ? maior + 1 : 0;
        if (resumo->tam_hist) {
            size_t *menor = mt_realloc(hist, resumo->tam_hist * sizeof *hist);
            resumo->hist = menor ? menor : hist;   /* encolher pode falhar: fica o maior */
        } else {
            mt_free(hist);
        }
    }
    *out = g;
    return CUSTO_OK;
}

void custo_resumo_liberar(CustoResumo *r)
{
    mt_free(r->hist);
    memset(r, 0, sizeof *r);
}
