/*
 * Testes das estruturas (#13): grafos de resultado conhecido — linha, ciclo,
 * estrela, desconexo e bipartido — montados nas duas representações e nas
 * visões direcionada e simetrizada.
 *
 * Para cada grafo são conferidos o número de arestas, o grau de cada vértice
 * (valores esperados escritos à mão) e, par a par, a existência de cada
 * aresta e a lista de vizinhos em ordem crescente.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_estruturas && ./bin/test_estruturas
 */
#include "graph.h"
#include "memtrack.h"

#include <stdio.h>
#include <stdlib.h>

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

#define MAX_V 8

typedef struct {
    size_t u, v;
} Par;

typedef struct {
    const char *nome;
    size_t n;
    Par arestas[16];
    size_t m;
    /* resultado esperado, escrito à mão */
    size_t m_dir, m_sim;          /* arestas em cada visão */
    size_t grau_dir[MAX_V];       /* grau de saída */
    size_t grau_sim[MAX_V];
} Caso;

static const Caso CASOS[] = {
    {"linha 0-1-2-3-4-5", 6, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}}, 5,
     5, 5, {1, 1, 1, 1, 1, 0}, {1, 2, 2, 2, 2, 1}},
    {"ciclo 0->1->2->3->4->5->0", 6, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0}}, 6,
     6, 6, {1, 1, 1, 1, 1, 1}, {2, 2, 2, 2, 2, 2}},
    /* todos seguem o centro 0, e o centro segue de volta só a folha 1 */
    {"estrela de centro 0", 7, {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {0, 1}}, 7,
     7, 6, {1, 1, 1, 1, 1, 1, 1}, {6, 1, 1, 1, 1, 1, 1}},
    /* {0,1,2} em triângulo, {3,4} e o vértice isolado 5 */
    {"desconexo", 6, {{0, 1}, {1, 2}, {2, 0}, {3, 4}}, 4,
     4, 4, {1, 1, 1, 1, 0, 0}, {2, 2, 2, 1, 1, 0}},
    /* bipartido completo K(2,3): lados {0,1} e {2,3,4}, arestas do lado menor
     * para o maior */
    {"bipartido K(2,3)", 5, {{0, 2}, {0, 3}, {0, 4}, {1, 2}, {1, 3}, {1, 4}}, 6,
     6, 6, {3, 3, 0, 0, 0}, {3, 3, 2, 2, 2}},
};
#define NUM_CASOS (sizeof CASOS / sizeof CASOS[0])

/* Existe u -> v no caso, segundo a lista de arestas e a visão? */
static int esperada(const Caso *c, GraphView view, size_t u, size_t v)
{
    for (size_t i = 0; i < c->m; i++) {
        if (c->arestas[i].u == u && c->arestas[i].v == v)
            return 1;
        if (view == GRAPH_SYMMETRIC && c->arestas[i].u == v && c->arestas[i].v == u)
            return 1;
    }
    return 0;
}

static void conferir(const Caso *c, GraphRep rep, GraphView view)
{
    Graph *g;
    CHECK(graph_create(c->n, rep, view, &g) == GRAPH_OK);
    for (size_t i = 0; i < c->m; i++)
        CHECK(graph_add_edge(g, c->arestas[i].u, c->arestas[i].v) == GRAPH_OK);

    const size_t *grau = view == GRAPH_DIRECTED ? c->grau_dir : c->grau_sim;
    size_t m = view == GRAPH_DIRECTED ? c->m_dir : c->m_sim;
    int ok_m = graph_num_vertices(g) == c->n && graph_num_edges(g) == m;
    int ok_grau = 1, ok_aresta = 1, ok_viz = 1;

    for (size_t u = 0; u < c->n; u++) {
        ok_grau &= (graph_degree(g, u) == grau[u]);

        /* os vizinhos devolvidos são exatamente os esperados, em ordem */
        GraphIter it;
        size_t w, proximo = 0, contados = 0;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next(&it, &w)) {
            while (proximo < c->n && !esperada(c, view, u, proximo))
                proximo++;
            ok_viz &= (w == proximo);
            proximo++;
            contados++;
        }
        ok_viz &= (contados == grau[u]);

        for (size_t v = 0; v < c->n; v++)
            ok_aresta &= (graph_has_edge(g, u, v) == esperada(c, view, u, v));
    }

    if (!(ok_m && ok_grau && ok_aresta && ok_viz))
        fprintf(stderr, "  caso '%s' (%s, %s):\n", c->nome, graph_rep_name(rep),
                graph_view_name(view));
    CHECK(ok_m);
    CHECK(ok_grau);
    CHECK(ok_aresta);
    CHECK(ok_viz);
    graph_destroy(g);
}

int main(void)
{
    for (size_t k = 0; k < NUM_CASOS; k++)
        for (int rep = GRAPH_LIST; rep <= GRAPH_MATRIX; rep++) {
            conferir(&CASOS[k], (GraphRep)rep, GRAPH_DIRECTED);
            conferir(&CASOS[k], (GraphRep)rep, GRAPH_SYMMETRIC);
        }

    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
