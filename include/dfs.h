#ifndef DFS_H
#define DFS_H

#include "graph.h"

#include <stddef.h>

/*
 * Busca em profundidade iterativa (#15).
 *
 * Pilha explícita no heap, sem recursão: com dezenas de milhares de vértices,
 * a recursão estouraria a pilha de chamadas. Registra o tempo de descoberta e
 * o de finalização de cada vértice, com um único relógio que avança a cada
 * evento (de 1 a 2V): o intervalo [descoberta, finalização] de um vértice
 * contém os de todos os seus descendentes (teorema dos parênteses).
 *
 * Percorre o grafo na visão em que ele foi construído (na direcionada, segue
 * o sentido das arestas), com vizinhos em ordem crescente de índice, então o
 * resultado é idêntico com lista e matriz.
 */

/* Núcleo: DFS a partir de `inicial`, que não pode ter sido descoberto ainda.
 * Marca os vértices alcançáveis em `descoberta` e `finalizacao` (V posições,
 * zero = ainda não descoberto), avançando o relógio `*tempo`. Chamadas
 * sucessivas com o mesmo relógio montam uma floresta. Devolve 0 se faltar
 * memória para a pilha. */
int graph_dfs_iterativa(const Graph *g, size_t inicial, size_t *descoberta, size_t *finalizacao,
                        size_t *tempo);

typedef struct {
    size_t n;                 /* vértices do grafo */
    size_t *descoberta;       /* tempo de descoberta de cada vértice, de 1 a 2V */
    size_t *finalizacao;      /* tempo de finalização */
    size_t arvores;           /* árvores da floresta da DFS */
    size_t alcancados;        /* vértices da árvore da origem, incluindo ela */
} DfsResultado;

typedef enum {
    DFS_OK = 0,
    DFS_ERRO_MEMORIA,
    DFS_ERRO_ORIGEM           /* origem fora de 0..V-1 */
} DfsStatus;

/* Floresta da DFS: a primeira árvore parte de `origem`; as seguintes, dos
 * vértices ainda não descobertos, em ordem de índice. Em erro, `r` fica
 * zerado e não precisa ser liberado. */
DfsStatus dfs_executar(const Graph *g, size_t origem, DfsResultado *r);

void dfs_liberar(DfsResultado *r);   /* aceita resultado zerado */

#endif
