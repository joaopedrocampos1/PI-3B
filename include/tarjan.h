#ifndef TARJAN_H
#define TARJAN_H

#include "graph.h"

#include <stddef.h>

/*
 * Pontes e vértices de articulação (#21), por Tarjan, na visão simetrizada.
 *
 * Um vértice de articulação é um usuário cuja remoção desconecta a rede em
 * que ele está; uma ponte é uma ligação com o mesmo efeito. No domínio: são
 * os usuários-ponte e os canais críticos de disseminação.
 *
 * DFS iterativa, com pilha explícita, que calcula para cada vértice num (ordem
 * de descoberta) e low (menor num alcançável pela subárvore com uma aresta de
 * retorno). Para o filho c de p na árvore da DFS:
 *   low[c] >= num[p]   p é articulação (se não for a raiz da árvore)
 *   low[c] >  num[p]   {p, c} é ponte
 * A raiz é articulação se tiver mais de um filho.
 *
 * Impacto de fragmentação. Remover a articulação v parte o componente dela em
 * pedaços: um para cada filho c com low[c] >= num[v] (a subárvore de c) e,
 * se v não for raiz, o resto do componente. O impacto de v é o número de
 * vértices que deixam de alcançar o maior pedaço restante, ou seja, os
 * usuários que ficam isolados do grosso da rede sem ele. Calculado na mesma
 * DFS, a partir do tamanho das subárvores. Custo O(V + E) com lista e
 * O(V² / 64 + E) com a matriz de bits.
 *
 * Só tem sentido nas visões sem sentido (simetrizada ou recíproca).
 */

typedef struct {
    size_t u, v;              /* índices das pontas, com u < v */
} Ponte;

typedef struct {
    size_t v;                 /* índice do vértice de articulação */
    size_t impacto;           /* vértices que perdem o maior pedaço sem ele */
    size_t pedacos;           /* em quantos pedaços o componente se parte */
} Articulacao;

typedef struct {
    size_t n;                       /* vértices do grafo */
    unsigned char *eh_articulacao;  /* 1 para os vértices de articulação */
    Articulacao *articulacoes;      /* ordenadas por impacto decrescente; em empate,
                                       por índice crescente */
    size_t num_articulacoes;
    Ponte *pontes;                  /* na ordem em que a DFS as encontra */
    size_t num_pontes;
} TarjanResultado;

typedef enum {
    TARJAN_OK = 0,
    TARJAN_ERRO_MEMORIA
} TarjanStatus;

/* Em erro, `r` fica zerado e não precisa ser liberado. */
TarjanStatus tarjan_executar(const Graph *g, TarjanResultado *r);

void tarjan_liberar(TarjanResultado *r);   /* aceita resultado zerado */

#endif
