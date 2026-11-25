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
 *
 * Cada representação tem duas tabelas: uma sem peso, a da Fase I, e uma com
 * peso (#64). A tabela é o que distingue um grafo com peso de um sem, então
 * struct Graph não ganha nenhum campo e o grafo sem peso ocupa o mesmo que
 * antes. Na tabela sem peso, add_weighted, weight e next_weighted são NULL.
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

    /* Só nas tabelas com peso. add_weighted insere u -> v com o peso, ou
     * troca o peso se a aresta já existe: 1 se inseriu, 0 se só trocou,
     * -1 sem memória. weight exige que u -> v exista. next_weighted é o
     * next, devolvendo também o peso. */
    int (*add_weighted)(void *data, size_t n, size_t u, size_t v, float peso);
    float (*weight)(const void *data, size_t n, size_t u, size_t v);
    int (*next_weighted)(const void *data, size_t n, size_t v, size_t *pos, size_t *w,
                         float *peso);
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
extern const GraphOps GRAPH_LIST_WEIGHTED_OPS;     /* graph_list.c   (#64) */
extern const GraphOps GRAPH_MATRIX_WEIGHTED_OPS;   /* graph_matrix.c (#64) */

#endif
