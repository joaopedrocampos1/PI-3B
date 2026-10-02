#ifndef GRAPH_H
#define GRAPH_H

#include "edgelist.h"
#include "idmap.h"

#include <stddef.h>

/*
 * TAD Grafo (#9): interface única para as duas representações (RF02).
 *
 * Os algoritmos só enxergam este header. Se por baixo é lista ou matriz de
 * adjacência, é escolhido em graph_create e não muda mais nada no código de
 * quem usa o grafo.
 *
 * Vértices são índices densos 0..V-1. O ID original de cada um fica no IdMap
 * usado para construir o grafo (graph_build).
 *
 * Três visões da mesma base de arestas, porque os algoritmos não concordam
 * sobre direcionamento:
 *
 *   GRAPH_DIRECTED    "A segue B" é a aresta A -> B.
 *                     BFS e graus de separação (#17), ciclos (#19).
 *   GRAPH_SYMMETRIC   "A segue B" liga A e B nos dois sentidos.
 *                     Bipartição (#20), pontes e articulação (#21).
 *   GRAPH_RECIPROCAL  só os pares que se seguem mutuamente, sem sentido.
 *                     Clique Máxima (Fase II).
 *
 * Nas visões sem sentido, cada aresta {u, v} aparece como vizinha nas duas
 * pontas, mas conta uma vez só em graph_num_edges.
 *
 * Vizinhos são percorridos em ordem crescente de índice nas duas
 * representações. É isso que garante resultado idêntico com lista e matriz (#12).
 *
 * Toda a memória passa pelo memtrack (RF03).
 */

typedef enum {
    GRAPH_LIST,
    GRAPH_MATRIX
} GraphRep;

typedef enum {
    GRAPH_DIRECTED,
    GRAPH_SYMMETRIC,
    GRAPH_RECIPROCAL
} GraphView;

typedef enum {
    GRAPH_OK = 0,
    GRAPH_ERR_MEMORY,
    GRAPH_ERR_UNAVAILABLE   /* representação ainda não implementada */
} GraphStatus;

typedef struct Graph Graph;   /* opaco: os campos ficam em graph_internal.h */

/* Percorre os vizinhos de um vértice. Fica na pilha de quem chama, o que
 * permite à DFS iterativa guardar um por nível da pilha explícita.
 * Os campos são internos: use só graph_neighbors_begin / _next. */
typedef struct {
    const Graph *g;
    size_t v;
    size_t pos;
} GraphIter;

/* Grafo vazio com `n` vértices e nenhuma aresta. Em erro, *out fica NULL. */
GraphStatus graph_create(size_t n, GraphRep rep, GraphView view, Graph **out);

/* Constrói a visão `view` a partir das arestas lidas.
 *
 * Os IDs originais são mapeados em `ids`, que deve vir iniciado (idmap_iniciar)
 * e pode já conter IDs: passar o mesmo IdMap para construir as três visões
 * garante que um vértice tenha o mesmo índice em todas. O grafo terá ids->n
 * vértices, e `ids` continua sendo de quem chamou.
 *
 * A visão recíproca inclui vértices sem nenhuma aresta mútua, isolados. */
GraphStatus graph_build(const EdgeList *el, IdMap *ids, GraphRep rep, GraphView view,
                        Graph **out);

void graph_destroy(Graph *g);   /* aceita NULL */

/* Insere u -> v (ou {u, v} nas visões sem sentido). Laços e arestas já
 * presentes são ignorados. Exige u, v < graph_num_vertices(g).
 * Em GRAPH_ERR_MEMORY o grafo pode ficar com só um dos sentidos de {u, v}:
 * deve ser destruído. */
GraphStatus graph_add_edge(Graph *g, size_t u, size_t v);

/* 1 se existe u -> v (ou {u, v}), 0 caso contrário. */
int graph_has_edge(const Graph *g, size_t u, size_t v);

/* Quantidade de vizinhos de v: grau de saída na visão direcionada, grau nas
 * visões sem sentido. */
size_t graph_degree(const Graph *g, size_t v);

size_t    graph_num_vertices(const Graph *g);
size_t    graph_num_edges(const Graph *g);
GraphRep  graph_rep(const Graph *g);
GraphView graph_view(const Graph *g);

/* Uso:
 *     GraphIter it;
 *     size_t w;
 *     graph_neighbors_begin(g, v, &it);
 *     while (graph_neighbors_next(&it, &w))
 *         ...
 * O grafo não pode ser alterado durante o percurso. */
void graph_neighbors_begin(const Graph *g, size_t v, GraphIter *it);
int  graph_neighbors_next(GraphIter *it, size_t *w);

/* "lista" / "matriz" e "direcionada" / "simetrizada" / "reciproca", para
 * mensagens e para o log. */
const char *graph_rep_name(GraphRep rep);
const char *graph_view_name(GraphView view);

#endif
