#ifndef COMPONENTES_H
#define COMPONENTES_H

#include "graph.h"

#include <stddef.h>

/*
 * Componentes conexos (#18).
 *
 * O que é um componente depende da visão em que o grafo foi construído:
 *
 *   GRAPH_DIRECTED    fortemente conexos: u e v estão no mesmo componente se
 *                     cada um alcança o outro seguindo o sentido das arestas.
 *                     Dentro de um componente, a publicação de qualquer
 *                     usuário pode chegar a qualquer outro por repasses; um
 *                     componente unitário é um usuário cujo conteúdo não volta
 *                     a circular até ele.
 *   GRAPH_SYMMETRIC   fracamente conexos: os componentes da rede ignorando o
 *                     sentido. Respondem se a rede é uma peça só.
 *   GRAPH_RECIPROCAL  componentes da rede de seguimento mútuo.
 *
 * Algoritmo de Tarjan, iterativo: uma única DFS com pilha explícita no heap,
 * sem recursão (que estouraria a pilha de chamadas com dezenas de milhares
 * de vértices) e sem precisar do grafo transposto. Nas visões sem sentido,
 * toda aresta vale nos dois sentidos, e os componentes fortes coincidem com
 * os conexos. Custo O(V + E) com lista e O(V² / 64 + E) com a matriz de bits.
 *
 * Os componentes são numerados na ordem em que o Tarjan os fecha. Como os
 * vizinhos são visitados em ordem de índice, a numeração é a mesma com lista
 * e com matriz.
 */

typedef struct {
    size_t n;                 /* vértices do grafo */
    size_t num_componentes;
    size_t *componente;       /* componente de cada vértice: 0..num_componentes-1 */
    size_t *tamanho;          /* vértices de cada componente */
    size_t gigante;           /* o maior componente (o de menor número, se empatar);
                                 SIZE_MAX se o grafo não tem vértices */
    size_t tamanho_gigante;
    size_t unitarios;         /* componentes com um único vértice */
} ComponentesResultado;

/* Uma faixa da distribuição de tamanhos. */
typedef struct {
    size_t tamanho;           /* vértices por componente */
    size_t quantidade;        /* componentes com esse tamanho */
} ComponentesFaixa;

typedef enum {
    COMPONENTES_OK = 0,
    COMPONENTES_ERRO_MEMORIA
} ComponentesStatus;

/* Calcula os componentes de `g`. Em erro, `r` fica zerado e não precisa ser
 * liberado. */
ComponentesStatus componentes_executar(const Graph *g, ComponentesResultado *r);

/* Distribuição dos tamanhos, do maior tamanho para o menor: grava em *faixas
 * um vetor de *k faixas, alocado pelo memtrack (liberar com mt_free). */
ComponentesStatus componentes_distribuicao(const ComponentesResultado *r,
                                           ComponentesFaixa **faixas, size_t *k);

void componentes_liberar(ComponentesResultado *r);   /* aceita resultado zerado */

#endif
