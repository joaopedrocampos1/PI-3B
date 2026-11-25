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
 *
 * Arestas com peso (Fase II, #64): um grafo criado com graph_create_weighted
 * guarda um peso float por aresta; um criado com graph_create/graph_build não
 * guarda peso nenhum e ocupa exatamente o mesmo que na Fase I. Para os
 * algoritmos, o grafo sem peso responde como se todo peso fosse 1. Na matriz,
 * o peso custa um float por par de vértices (cerca de 400 MB no recorte de
 * 10 mil vértices, contra 12,5 MB da matriz de bits): é o preço da matriz
 * numa rede esparsa, e é parte da comparação lista x matriz da Fase II.
 */

#include <math.h>   /* INFINITY, devolvido por graph_edge_weight */

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
    GRAPH_ERR_UNAVAILABLE,    /* representação ainda não implementada */
    GRAPH_ERR_NOT_WEIGHTED,   /* aresta com peso num grafo criado sem peso */
    GRAPH_ERR_INVALID_WEIGHT  /* peso NaN ou infinito */
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

/* Como graph_create, mas o grafo guarda um peso por aresta (#64). */
GraphStatus graph_create_weighted(size_t n, GraphRep rep, GraphView view, Graph **out);

/* 1 se o grafo foi criado com peso, 0 caso contrário. */
int graph_is_weighted(const Graph *g);

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

/* Insere u -> v (ou {u, v}) com peso `peso`, num grafo criado com peso.
 * Diferente de graph_add_edge, inserir de novo uma aresta presente troca o
 * peso dela (nas visões sem sentido, nas duas pontas), sem contar outra
 * aresta. Laços são ignorados. graph_add_edge num grafo com peso insere
 * com peso 1.
 * Erros, sem alterar o grafo: GRAPH_ERR_NOT_WEIGHTED se o grafo foi criado
 * sem peso, GRAPH_ERR_INVALID_WEIGHT se o peso for NaN ou infinito. Pesos
 * negativos são aceitos aqui; quem não os admite (o Dijkstra) os recusa. */
GraphStatus graph_add_weighted_edge(Graph *g, size_t u, size_t v, float peso);

/* Peso de u -> v (ou {u, v}); INFINITY se a aresta não existe. Num grafo
 * sem peso, 1 para toda aresta presente. */
float graph_edge_weight(const Graph *g, size_t u, size_t v);

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

/* Como graph_neighbors_next, e devolve também o peso da aresta até *w
 * (1 num grafo sem peso). Os dois podem ser misturados no mesmo percurso. */
int  graph_neighbors_next_weighted(GraphIter *it, size_t *w, float *peso);

/* "lista" / "matriz" e "direcionada" / "simetrizada" / "reciproca", para
 * mensagens e para o log. */
const char *graph_rep_name(GraphRep rep);
const char *graph_view_name(GraphView view);

#endif
