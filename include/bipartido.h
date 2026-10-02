#ifndef BIPARTIDO_H
#define BIPARTIDO_H

#include "graph.h"

#include <stddef.h>

/*
 * Verificação de bipartição (#20), na visão simetrizada.
 *
 * Um grafo é bipartido se os vértices cabem em dois lados sem nenhuma aresta
 * dentro de um mesmo lado. No domínio: se a rede se divide em dois grupos que
 * só se relacionam com o outro grupo, nunca entre si.
 *
 * Coloração em duas cores por BFS, componente por componente. O grafo é
 * bipartido se e somente se nenhuma aresta liga dois vértices da mesma cor.
 * Quando há uma, o ciclo ímpar responsável é reconstruído subindo pela árvore
 * do BFS a partir das duas pontas da aresta até elas se encontrarem.
 *
 * A coloração cobre o grafo inteiro mesmo depois do primeiro conflito, para
 * que o custo seja O(V + E) em qualquer grafo (O(V² / 64 + E) com a matriz de
 * bits), como o protocolo experimental mede. O ciclo ímpar devolvido é o do
 * primeiro conflito encontrado.
 *
 * O resultado só tem sentido nas visões sem sentido (simetrizada ou
 * recíproca): na direcionada, cada vértice só enxergaria as arestas de saída.
 */

typedef struct {
    size_t n;                 /* vértices do grafo */
    int bipartido;            /* 1 se nenhuma aresta liga vértices da mesma cor */
    unsigned char *cor;       /* 0 ou 1 para cada vértice */
    size_t lado[2];           /* vértices de cada cor; só têm sentido se bipartido */
    size_t *ciclo_impar;      /* se não for bipartido: os vértices de um ciclo ímpar,
                                 em ordem (cada um é vizinho do seguinte, e o último
                                 do primeiro); NULL se for bipartido */
    size_t tam_ciclo;         /* vértices do ciclo ímpar; 0 se bipartido */
} BipartidoResultado;

typedef enum {
    BIPARTIDO_OK = 0,
    BIPARTIDO_ERRO_MEMORIA
} BipartidoStatus;

/* Em erro, `r` fica zerado e não precisa ser liberado. */
BipartidoStatus bipartido_executar(const Graph *g, BipartidoResultado *r);

void bipartido_liberar(BipartidoResultado *r);   /* aceita resultado zerado */

#endif
