#ifndef BFS_H
#define BFS_H

#include "graph.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Busca em largura genérica (#14).
 *
 * Percorre o grafo na visão em que ele foi construído: na direcionada, segue
 * o sentido das arestas (u -> v). Como "A segue B" é a aresta A -> B, a
 * distância de A até B é o número de repasses para uma publicação de B
 * chegar a A.
 *
 * Fila autoral, em vetor: cada vértice entra nela no máximo uma vez, então
 * um vetor de V posições basta, sem realocação. Custo O(V + E) com lista e
 * O(V² / 64 + E) com a matriz de bits.
 *
 * Os vizinhos são visitados em ordem crescente de índice, então o resultado
 * (inclusive os predecessores) é idêntico com lista e matriz.
 */

#define BFS_INF     SIZE_MAX   /* distância de um vértice não alcançado */
#define BFS_NENHUM  SIZE_MAX   /* predecessor de uma origem ou de um não alcançado */

typedef struct {
    size_t n;                  /* vértices do grafo */
    unsigned char *visitado;   /* 1 se alcançado */
    size_t *dist;              /* saltos desde a origem mais próxima, ou BFS_INF */
    size_t *pred;              /* vértice anterior no caminho mínimo, ou BFS_NENHUM */

    size_t alcancados;         /* vértices alcançados, incluindo as origens */
    size_t dist_max;           /* maior distância encontrada: a excentricidade
                                  da origem, restrita aos alcançados */
    double dist_media;         /* média das distâncias dos alcançados, sem as
                                  origens; 0 se ninguém além delas */
} BfsResultado;

typedef enum {
    BFS_OK = 0,
    BFS_ERRO_MEMORIA,
    BFS_ERRO_ORIGEM            /* origem fora de 0..V-1, ou nenhuma origem */
} BfsStatus;

/* BFS a partir de `origem`. Em erro, `r` fica zerado e não precisa ser
 * liberado. */
BfsStatus bfs_executar(const Graph *g, size_t origem, BfsResultado *r);

/* BFS a partir de várias origens ao mesmo tempo, todas com distância 0: a
 * distância de cada vértice passa a ser até a origem mais próxima. Origens
 * repetidas são aceitas. */
BfsStatus bfs_executar_multi(const Graph *g, const size_t *origens, size_t k, BfsResultado *r);

/* Caminho mínimo da origem até `destino`, gravado em `caminho` na ordem
 * origem -> destino. Devolve o número de vértices do caminho (distância + 1),
 * ou 0 se o destino não foi alcançado. Se o caminho tiver mais que `max`
 * vértices, nada é gravado, mas o tamanho é devolvido do mesmo jeito: chame
 * com max = 0 para saber quanto alocar. */
size_t bfs_caminho(const BfsResultado *r, size_t destino, size_t *caminho, size_t max);

void bfs_liberar(BfsResultado *r);   /* aceita resultado zerado */

#endif
