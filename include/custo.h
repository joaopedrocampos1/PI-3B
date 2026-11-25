#ifndef CUSTO_H
#define CUSTO_H

#include "graph.h"

#include <stddef.h>

/*
 * Engajamento e confiança como custo (#65).
 *
 * O ego-Twitter não traz engajamento nem confiança: cada linha é só "A segue
 * B". O peso de cada aresta é derivado da estrutura, pela similaridade entre
 * os dois usuários, no modelo escolhido pelo grupo:
 *
 *     custo(A -> B) = 1 / (1 + c)
 *     c = |N(A) ∩ N(B)|, mais 1 se A e B se seguem mutuamente
 *
 * N(x) são os vizinhos de x na visão simetrizada: quem x segue e quem segue
 * x, o "círculo" de x. Como não há laços, nem A nem B entram na interseção.
 * Quanto mais contatos em comum, maior a confiança e menor o atrito. O custo
 * fica em (0, 1], vale 1 quando não há nenhum contato em comum e nunca divide
 * por zero. Ele é simétrico: custo(A -> B) = custo(B -> A) quando as duas
 * arestas existem.
 *
 * Sentido. "A segue B" é a aresta A -> B da visão direcionada, mas a
 * publicação anda de B para A (a mesma convenção da #17). O grafo com peso
 * pode ser montado nos dois sentidos, e quem chama escolhe explicitamente:
 *
 *   CUSTO_SENTIDO_PROPAGACAO   B -> A: o caminho de uma campanha. Um Dijkstra
 *                              (#66) a partir de quem publica segue as
 *                              arestas deste grafo.
 *   CUSTO_SENTIDO_SEGUE        A -> B: as arestas da visão direcionada, com
 *                              peso.
 *
 * Custo de cálculo. Para cada aresta, os vizinhos do vértice de menor grau
 * são consultados no outro com graph_has_edge: O(Σ min(grau(A), grau(B)))
 * consultas, cada uma O(log grau) com a lista e O(1) com a matriz.
 *
 * Só usa graph_degree, graph_has_edge e o percurso de vizinhos, então os
 * custos saem idênticos com lista e com matriz. Toda a memória passa pelo
 * memtrack (RF03).
 */

typedef enum {
    CUSTO_SENTIDO_PROPAGACAO,
    CUSTO_SENTIDO_SEGUE
} CustoSentido;

typedef enum {
    CUSTO_OK = 0,
    CUSTO_ERRO_MEMORIA,
    CUSTO_ERRO_VISAO      /* dir não é direcionado, sim não é simetrizado,
                             ou os dois têm números de vértices diferentes */
} CustoStatus;

/* Distribuição de c pelas arestas da visão direcionada. */
typedef struct {
    size_t arestas;           /* arestas A -> B examinadas */
    size_t mutuas;            /* delas, quantas têm a volta B -> A */
    size_t *hist;             /* hist[c]: arestas com c contatos (mútua conta 1);
                                 hist[0] são as de custo 1 */
    size_t tam_hist;          /* maior c + 1; 0 se não há arestas */
} CustoResumo;

/* Contatos em comum de a e b na visão simetrizada `sim`: |N(a) ∩ N(b)|. */
size_t custo_comuns(const Graph *sim, size_t a, size_t b);

/* Custo da aresta a -> b ("a segue b"), que precisa existir em `dir`. */
float custo_aresta(const Graph *dir, const Graph *sim, size_t a, size_t b);

/* Monta em *out o grafo com peso, direcionado, com uma aresta para cada
 * aresta de `dir`, no sentido pedido, e o custo como peso. `dir` deve ser a
 * visão direcionada e `sim` a simetrizada do mesmo conjunto de arestas, com
 * os mesmos índices (o mesmo IdMap em graph_build). A representação de *out
 * é `rep`, independente das de dir e sim.
 *
 * Se `resumo` não for NULL, recebe a distribuição dos custos; libere com
 * custo_resumo_liberar. Em erro, *out fica NULL e o resumo zerado. */
CustoStatus custo_construir(const Graph *dir, const Graph *sim, GraphRep rep,
                            CustoSentido sentido, Graph **out, CustoResumo *resumo);

void custo_resumo_liberar(CustoResumo *r);   /* aceita resumo zerado */

#endif
