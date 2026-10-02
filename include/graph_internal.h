#ifndef GRAPH_INTERNAL_H
#define GRAPH_INTERNAL_H

#include "graph.h"

/*
 * Parte interna do TAD Grafo, compartilhada por graph.c e pelas
 * representações (graph_list.c, graph_matrix.c). Algoritmos NÃO incluem
 * este header: só graph.h.
 *
 * Cada representação fornece uma tabela de operações sobre arestas
 * direcionadas u -> v. O sentido, a visão e a contagem de arestas são
 * tratados em graph.c, uma vez só para as duas.
 */

typedef struct {
    /* Estrutura vazia com n vértices; NULL se faltar memória. */
    void *(*create)(size_t n);
    void (*destroy)(void *data, size_t n);

    /* Insere u -> v. Retorna 1 se inseriu, 0 se já existia, -1 sem memória. */
    int (*add)(void *data, size_t n, size_t u, size_t v);
    int (*has)(const void *data, size_t n, size_t u, size_t v);
    size_t (*degree)(const void *data, size_t n, size_t v);

    /* Próximo vizinho de v a partir da posição *pos, em ordem crescente de
     * índice. *pos começa em 0 e o significado dos valores seguintes é
     * da representação. Retorna 0 quando acabam os vizinhos. */
    int (*next)(const void *data, size_t n, size_t v, size_t *pos, size_t *w);
} GraphOps;

struct Graph {
    const GraphOps *ops;
    void *data;
    GraphRep rep;
    GraphView view;
    size_t n;   /* vértices */
    size_t m;   /* arestas; nas visões sem sentido, {u, v} conta uma vez */
};

extern const GraphOps GRAPH_LIST_OPS;     /* graph_list.c   (#10) */
extern const GraphOps GRAPH_MATRIX_OPS;   /* graph_matrix.c (#11) */

#endif
