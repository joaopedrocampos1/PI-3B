#ifndef CLIQUE_H
#define CLIQUE_H

#include "graph.h"

#include <stddef.h>

/*
 * Clique Máxima exata (#67), na visão recíproca.
 *
 * Pergunta de negócio: onde estão as câmaras de eco perfeitas, grupos em que
 * todos se seguem mutuamente? Na visão recíproca, uma aresta {u, v} existe só
 * quando u segue v e v segue u, então uma clique é exatamente esse grupo.
 *
 * Algoritmo: branch and bound com limitante por coloração gulosa (MCQ, de
 * Tomita e Seki). A cada nó da busca, os candidatos são coloridos de forma
 * gulosa; vértices da mesma cor não são vizinhos entre si, então k cores
 * limitam a clique que ainda cabe nos candidatos a k vértices. Um ramo é
 * podado quando a clique atual mais esse limitante não supera a melhor já
 * encontrada.
 *
 * A busca é iterativa, com pilha explícita de níveis. A profundidade nunca
 * passa do tamanho da clique máxima + 1.
 *
 * O resultado só usa graph_degree, graph_has_edge e o percurso de vizinhos,
 * por isso sai idêntico com lista e com matriz (#12). Toda a memória passa
 * pelo memtrack (RF03).
 *
 * O problema é NP-difícil. Com `limite_ms` > 0, a busca para quando o tempo
 * estoura e devolve a melhor clique encontrada até ali, com exato = 0: ela é
 * uma clique de verdade, mas pode não ser a máxima. Assim a bateria mede até
 * onde a solução exata é viável.
 *
 * Só tem sentido nas visões sem sentido (recíproca ou simetrizada): na
 * direcionada, graph_has_edge(u, v) e graph_has_edge(v, u) diferem.
 */

typedef struct {
    size_t n;                   /* vértices do grafo */
    size_t *clique;             /* vértices da clique, em ordem crescente de
                                   índice; NULL se o grafo não tiver vértices */
    size_t tam;                 /* vértices da clique */
    unsigned long long nos;     /* nós visitados na busca: cada vértice
                                   acrescentado a uma clique parcial conta um */
    int exato;                  /* 1 se a busca terminou e a clique é máxima;
                                   0 se parou pelo limite de tempo */
} CliqueResultado;

typedef enum {
    CLIQUE_OK = 0,
    CLIQUE_ERRO_MEMORIA
} CliqueStatus;

/* `limite_ms` <= 0 desliga o limite de tempo.
 * Em erro, `r` fica zerado e não precisa ser liberado. */
CliqueStatus clique_exata(const Graph *g, double limite_ms, CliqueResultado *r);

void clique_liberar(CliqueResultado *r);   /* aceita resultado zerado */

#endif
