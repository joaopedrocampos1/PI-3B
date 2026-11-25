#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Dijkstra com fila de prioridade autoral (#66).
 *
 * Pergunta de negócio: qual é o caminho de influência de menor atrito para
 * espalhar uma campanha? O atrito de cada aresta é o peso dela (#65), e a
 * distância de um vértice é o atrito acumulado no melhor caminho desde a
 * origem.
 *
 * Segue o sentido das arestas na visão em que o grafo foi construído, como o
 * BFS. Num grafo sem peso, todo peso vale 1 e as distâncias são as do BFS.
 *
 * A fila é um heap binário mínimo indexado, escrito aqui: cada vértice entra
 * nele uma vez só, e uma distância melhor sobe o vértice no lugar (diminuir
 * a prioridade), em vez de inserir uma cópia. Custo O((V + E) log V) com
 * lista e O(V² / 64 + E log V) com a matriz.
 *
 * Desempate: o heap ordena por (distância, índice do vértice), e uma
 * distância só é trocada por outra estritamente menor. Com os vizinhos em
 * ordem de índice, o resultado, inclusive os predecessores, é idêntico com
 * lista e matriz. Entre dois caminhos de mesmo custo, fica aquele cujo
 * penúltimo vértice saiu primeiro da fila.
 *
 * Pesos negativos invalidam o Dijkstra: se a busca encontra uma aresta de
 * peso negativo, ela para com DIJKSTRA_ERRO_PESO_NEGATIVO.
 */

#define DIJKSTRA_INF     INFINITY   /* distância de um vértice não alcançado */
#define DIJKSTRA_NENHUM  SIZE_MAX   /* predecessor da origem ou de um não alcançado */

typedef struct {
    size_t n;            /* vértices do grafo */
    size_t origem;
    double *dist;        /* atrito acumulado desde a origem, ou DIJKSTRA_INF */
    size_t *pred;        /* vértice anterior no caminho de menor custo, ou DIJKSTRA_NENHUM */
    size_t alcancados;   /* vértices alcançados, incluindo a origem */
} DijkstraResultado;

typedef enum {
    DIJKSTRA_OK = 0,
    DIJKSTRA_ERRO_MEMORIA,
    DIJKSTRA_ERRO_ORIGEM,          /* origem fora de 0..V-1 */
    DIJKSTRA_ERRO_PESO_NEGATIVO    /* a busca alcançou uma aresta de peso negativo */
} DijkstraStatus;

/* Dijkstra a partir de `origem`. Em erro, `r` fica zerado e não precisa ser
 * liberado. */
DijkstraStatus dijkstra_executar(const Graph *g, size_t origem, DijkstraResultado *r);

/* Caminho de menor custo da origem até `destino`, gravado em `caminho` na
 * ordem origem -> destino. Devolve o número de vértices do caminho, ou 0 se
 * o destino não foi alcançado. Se o caminho tiver mais que `max` vértices,
 * nada é gravado, mas o tamanho é devolvido do mesmo jeito: chame com
 * max = 0 para saber quanto alocar. */
size_t dijkstra_caminho(const DijkstraResultado *r, size_t destino, size_t *caminho, size_t max);

void dijkstra_liberar(DijkstraResultado *r);   /* aceita resultado zerado */

#endif
