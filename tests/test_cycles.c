/*
 * Testes da detecção de ciclos (src/cycles.c), nas duas representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_cycles && ./bin/test_cycles
 */
#include "cycles.h"
#include "memtrack.h"

#include <stdio.h>
#include <stdlib.h>

static int falhas = 0;
static int verificacoes = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        verificacoes++;                                                      \
        if (!(cond)) {                                                       \
            falhas++;                                                        \
            fprintf(stderr, "  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

typedef struct {
    size_t u, v;
} Par;

static Graph *montar(GraphRep rep, size_t n, const Par *a, size_t m)
{
    Graph *g;
    if (graph_create(n, rep, GRAPH_DIRECTED, &g) != GRAPH_OK) {
        fprintf(stderr, "sem memória para o grafo de teste\n");
        exit(2);
    }
    for (size_t i = 0; i < m; i++)
        graph_add_edge(g, a[i].u, a[i].v);
    return g;
}

#define MONTAR(rep, n, arestas) montar(rep, n, arestas, sizeof arestas / sizeof *arestas)

/* O exemplo é um ciclo dirigido do grafo: vértices distintos, cada um com
 * aresta para o seguinte e o último com aresta para o primeiro. */
static int ciclo_valido(const Graph *g, const CiclosResultado *r)
{
    size_t k = r->tam_ciclo;
    if (!r->ciclo || k < 2)
        return 0;
    for (size_t i = 0; i < k; i++) {
        for (size_t j = 0; j < i; j++)
            if (r->ciclo[i] == r->ciclo[j])
                return 0;
        if (!graph_has_edge(g, r->ciclo[i], r->ciclo[(i + 1) % k]))
            return 0;
    }
    return 1;
}

static int classes_somam_m(const Graph *g, const CiclosResultado *r)
{
    return r->arvore + r->retorno + r->avanco + r->cruzada == graph_num_edges(g);
}

/* G1 da conferência manual. DFS a partir de 0: todas as arestas são de árvore,
 * menos 2 -> 0 e 5 -> 3 (retorno); depois, 9 -> 10 -> 11 (árvore). O primeiro
 * retorno é 2 -> 0, que fecha o ciclo 0 -> 1 -> 2. */
static void conferencia_g1(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 5},
               {5, 3}, {5, 6}, {6, 7}, {7, 8}, {9, 10}, {10, 11}};
    Graph *g = MONTAR(rep, 12, a);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(r.tem_ciclo);
    CHECK(r.arvore == 10 && r.retorno == 2 && r.avanco == 0 && r.cruzada == 0);
    CHECK(classes_somam_m(g, &r));
    CHECK(r.tam_ciclo == 3 && r.ciclo[0] == 0 && r.ciclo[1] == 1 && r.ciclo[2] == 2);
    CHECK(ciclo_valido(g, &r));
    ciclos_liberar(&r);
    graph_destroy(g);
}

/* 0 -> 1 -> 2 e 0 -> 2: quando 0 -> 2 é examinada, 2 já terminou e foi
 * descoberto depois de 0. Aresta de avanço; sem ciclo. */
static void aresta_de_avanco(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {0, 2}};
    Graph *g = MONTAR(rep, 3, a);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(!r.tem_ciclo && r.ciclo == NULL && r.tam_ciclo == 0);
    CHECK(r.arvore == 2 && r.avanco == 1 && r.retorno == 0 && r.cruzada == 0);
    ciclos_liberar(&r);
    graph_destroy(g);
}

/* 0 -> 1 -> 2 -> 3 e 1 -> 3: avanço saindo de um vértice interno (o 1), não
 * da raiz. Distingue a ordem de descoberta de todos os vértices da árvore. */
static void avanco_de_vertice_interno(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {1, 3}};
    Graph *g = MONTAR(rep, 4, a);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(!r.tem_ciclo);
    CHECK(r.arvore == 3 && r.avanco == 1 && r.cruzada == 0);
    ciclos_liberar(&r);
    graph_destroy(g);
}

/* 0 -> 1, 0 -> 2 e 2 -> 1: quando 2 -> 1 é examinada, 1 já terminou e foi
 * descoberto antes de 2. Aresta cruzada; sem ciclo. */
static void aresta_cruzada(GraphRep rep)
{
    Par a[] = {{0, 1}, {0, 2}, {2, 1}};
    Graph *g = MONTAR(rep, 3, a);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(!r.tem_ciclo);
    CHECK(r.arvore == 2 && r.cruzada == 1 && r.avanco == 0 && r.retorno == 0);
    ciclos_liberar(&r);
    graph_destroy(g);
}

/* 1 -> 0: a DFS começa pelo 0, que não tem saída; depois pelo 1, cuja aresta
 * vai para uma árvore anterior. Cruzada entre árvores. */
static void cruzada_entre_arvores(GraphRep rep)
{
    Par a[] = {{1, 0}};
    Graph *g = MONTAR(rep, 2, a);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(!r.tem_ciclo && r.arvore == 0 && r.cruzada == 1);
    ciclos_liberar(&r);
    graph_destroy(g);
}

/* 0 <-> 1: o menor ciclo dirigido, com dois vértices (seguimento mútuo). */
static void ciclo_de_dois(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 0}};
    Graph *g = MONTAR(rep, 2, a);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(r.tem_ciclo && r.arvore == 1 && r.retorno == 1);
    CHECK(r.tam_ciclo == 2 && ciclo_valido(g, &r));
    ciclos_liberar(&r);
    graph_destroy(g);
}

/* Depois do primeiro ciclo, a classificação continua: o segundo ciclo, em
 * outra árvore, também é contado. */
static void continua_depois_do_primeiro(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 0}, {2, 3}, {3, 4}, {4, 2}};
    Graph *g = MONTAR(rep, 5, a);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(r.retorno == 2 && r.arvore == 3);
    CHECK(classes_somam_m(g, &r));
    CHECK(r.tam_ciclo == 2 && r.ciclo[0] == 0 && r.ciclo[1] == 1);   /* o primeiro */
    ciclos_liberar(&r);
    graph_destroy(g);
}

static void vazio(GraphRep rep)
{
    Graph *g;
    graph_create(0, rep, GRAPH_DIRECTED, &g);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(!r.tem_ciclo && r.arvore + r.retorno + r.avanco + r.cruzada == 0);
    ciclos_liberar(&r);
    graph_destroy(g);
}

/* Ciclo 0 -> 1 -> ... -> n-1 -> 0: a DFS desce n níveis, e o exemplo é o
 * ciclo inteiro. Com recursão, a pilha de chamadas estouraria. */
static void ciclo_profundo(GraphRep rep, size_t n)
{
    Graph *g;
    graph_create(n, rep, GRAPH_DIRECTED, &g);
    for (size_t v = 0; v < n; v++)
        graph_add_edge(g, v, (v + 1) % n);
    CiclosResultado r;
    CHECK(ciclos_executar(g, &r) == CICLOS_OK);
    CHECK(r.tem_ciclo && r.retorno == 1 && r.arvore == n - 1);
    CHECK(r.tam_ciclo == n && ciclo_valido(g, &r));
    ciclos_liberar(&r);
    graph_destroy(g);
}

static void rodar(GraphRep rep)
{
    conferencia_g1(rep);
    aresta_de_avanco(rep);
    avanco_de_vertice_interno(rep);
    aresta_cruzada(rep);
    cruzada_entre_arvores(rep);
    ciclo_de_dois(rep);
    continua_depois_do_primeiro(rep);
    vazio(rep);
}

int main(void)
{
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);
    ciclo_profundo(GRAPH_LIST, 100000);
    ciclo_profundo(GRAPH_MATRIX, 20000);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
