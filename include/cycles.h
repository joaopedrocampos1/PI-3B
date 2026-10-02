#ifndef CYCLES_H
#define CYCLES_H

#include "graph.h"

#include <stddef.h>

/*
 * Detecção de ciclos (#19), na visão direcionada.
 *
 * DFS iterativa, com pilha explícita, que classifica cada aresta u -> w pelo
 * estado de w no momento em que é examinada:
 *
 *   branco (não visitado)        árvore
 *   cinza (na pilha da DFS)      retorno: fecha um ciclo
 *   preto (já finalizado)        avanço, se u foi descoberto antes de w;
 *                                cruzada, caso contrário
 *
 * O grafo tem ciclo se e somente se a DFS encontra alguma aresta de retorno.
 * No domínio: um ciclo é um caminho de repasses que devolve a publicação a
 * quem a originou.
 *
 * Percorre o grafo inteiro, partindo dos vértices em ordem de índice, então
 * toda aresta é classificada exatamente uma vez: as quatro contagens somam
 * graph_num_edges. O ciclo de exemplo é o fechado pela primeira aresta de
 * retorno. Custo O(V + E) com lista e O(V² / 64 + E) com a matriz de bits.
 *
 * Só tem sentido na visão direcionada: nas visões sem sentido, toda aresta
 * de árvore seria vista de volta como retorno.
 */

typedef struct {
    int tem_ciclo;
    size_t arvore;            /* arestas de cada tipo */
    size_t retorno;
    size_t avanco;
    size_t cruzada;
    size_t *ciclo;            /* exemplo: v0 -> v1 -> ... -> v(k-1) -> v0, em índices;
                                 NULL se não há ciclo */
    size_t tam_ciclo;         /* vértices do exemplo; 0 se não há ciclo */
} CiclosResultado;

typedef enum {
    CICLOS_OK = 0,
    CICLOS_ERRO_MEMORIA
} CiclosStatus;

/* Em erro, `r` fica zerado e não precisa ser liberado. */
CiclosStatus ciclos_executar(const Graph *g, CiclosResultado *r);

void ciclos_liberar(CiclosResultado *r);   /* aceita resultado zerado */

#endif
