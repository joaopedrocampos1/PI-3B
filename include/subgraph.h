#ifndef SUBGRAPH_H
#define SUBGRAPH_H

#include "edgelist.h"
#include "idmap.h"

#include <stddef.h>

/*
 * Amostragem de subgrafos por BFS, para os testes de estresse com
 * N = 100, 250, 500 e 1.000 vértices.
 *
 * - O BFS ignora o sentido das arestas: "A segue B" liga A e B nos dois
 *   sentidos. Seguindo o sentido, a busca pararia em quem não segue ninguém.
 * - Os vizinhos de cada vértice são visitados em ordem crescente de índice
 *   (o índice do IdMap), o que torna a amostra reproduzível.
 * - A amostra são os N primeiros vértices descobertos, e o subgrafo devolvido
 *   contém todas as arestas originais (direcionadas) entre eles.
 */

typedef struct {
    const EdgeList *arestas;  /* arestas originais; precisam viver mais que o Amostrador */
    IdMap ids;                /* ID original <-> índice 0..V-1 */
    size_t *inicio;           /* vizinhos de v: vizinhos[inicio[v] .. inicio[v+1]-1] */
    size_t *vizinhos;         /* índices, sem repetição, em ordem crescente */
} Amostrador;

typedef enum {
    AMOSTRA_OK = 0,
    AMOSTRA_ERRO_MEMORIA,
    AMOSTRA_SEMENTE_INEXISTENTE,  /* a semente não é vértice do grafo */
    AMOSTRA_COMPONENTE_PEQUENO,   /* a semente alcança menos de N vértices */
    AMOSTRA_SEMENTES_INSUFICIENTES /* pediu mais sementes do que há vértices */
} AmostraStatus;

/* Prepara a lista de vizinhos (sem sentido) a partir das arestas lidas. */
AmostraStatus subgraph_preparar(Amostrador *a, const EdgeList *el);

/* Subgrafo com os `n` primeiros vértices descobertos por BFS a partir de
 * `semente` (ID original). Em caso de erro, `saida` fica vazia. */
AmostraStatus subgraph_bfs(const Amostrador *a, unsigned long long semente, size_t n,
                           EdgeList *saida);

/* Sorteia `k` vértices distintos para servirem de semente, gravando seus IDs
 * originais em `sementes`. O mesmo `semente_rng` produz sempre o mesmo
 * sorteio, em qualquer plataforma. */
AmostraStatus subgraph_sortear_sementes(const Amostrador *a, unsigned long long semente_rng,
                                        size_t k, unsigned long long *sementes);

void subgraph_liberar(Amostrador *a);

#endif
