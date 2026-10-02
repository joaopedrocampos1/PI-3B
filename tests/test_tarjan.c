/*
 * Testes de pontes e vértices de articulação (src/tarjan.c), nas duas
 * representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_tarjan && ./bin/test_tarjan
 */
#include "tarjan.h"
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
    if (graph_create(n, rep, GRAPH_SYMMETRIC, &g) != GRAPH_OK) {
        fprintf(stderr, "sem memória para o grafo de teste\n");
        exit(2);
    }
    for (size_t i = 0; i < m; i++)
        graph_add_edge(g, a[i].u, a[i].v);
    return g;
}

#define MONTAR(rep, n, arestas) montar(rep, n, arestas, sizeof arestas / sizeof *arestas)

static int tem_ponte(const TarjanResultado *r, size_t a, size_t b)
{
    size_t u = a < b ? a : b, v = a < b ? b : a;
    for (size_t i = 0; i < r->num_pontes; i++)
        if (r->pontes[i].u == u && r->pontes[i].v == v)
            return 1;
    return 0;
}

/* A k-ésima articulação do ranking é `v`, com esse impacto e pedaços. */
static int no_ranking(const TarjanResultado *r, size_t k, size_t v, size_t impacto, size_t pedacos)
{
    return k < r->num_articulacoes && r->articulacoes[k].v == v &&
           r->articulacoes[k].impacto == impacto && r->articulacoes[k].pedacos == pedacos;
}

/* G1 da conferência manual, visão simetrizada. Gabarito: pontes {2,3},
 * {5,6}, {6,7}, {7,8}, {9,10}, {10,11}; articulações 2, 3, 5, 6, 7, 10.
 * Impactos (vértices fora do maior pedaço restante), à mão:
 *   3: {0,1,2} e {4..8}  -> 3      5: {0..4} e {6,7,8} -> 3
 *   2: {0,1} e {3..8}    -> 2      6: {0..5} e {7,8}   -> 2
 *   7: {0..6} e {8}      -> 1     10: {9} e {11}       -> 1 */
static void conferencia_g1(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 5},
               {5, 3}, {5, 6}, {6, 7}, {7, 8}, {9, 10}, {10, 11}};
    Graph *g = MONTAR(rep, 12, a);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);

    CHECK(r.num_pontes == 6);
    CHECK(tem_ponte(&r, 2, 3) && tem_ponte(&r, 5, 6) && tem_ponte(&r, 6, 7));
    CHECK(tem_ponte(&r, 7, 8) && tem_ponte(&r, 9, 10) && tem_ponte(&r, 10, 11));
    CHECK(!tem_ponte(&r, 0, 1) && !tem_ponte(&r, 3, 4));   /* arestas de triângulo */

    CHECK(r.num_articulacoes == 6);
    int marcadas = 1;
    for (size_t v = 0; v < 12; v++)
        marcadas &= r.eh_articulacao[v] == (v == 2 || v == 3 || v == 5 || v == 6 || v == 7 || v == 10);
    CHECK(marcadas);
    CHECK(no_ranking(&r, 0, 3, 3, 2));
    CHECK(no_ranking(&r, 1, 5, 3, 2));
    CHECK(no_ranking(&r, 2, 2, 2, 2));
    CHECK(no_ranking(&r, 3, 6, 2, 2));
    CHECK(no_ranking(&r, 4, 7, 1, 2));
    CHECK(no_ranking(&r, 5, 10, 1, 2));
    tarjan_liberar(&r);
    graph_destroy(g);
}

/* G2: quadrado 0-1-2-3 com cauda 3-4. Ponte {3, 4}; articulação 3, que
 * separa só o 4. */
static void conferencia_g2(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {3, 4}};
    Graph *g = MONTAR(rep, 5, a);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == 1 && tem_ponte(&r, 3, 4));
    CHECK(r.num_articulacoes == 1 && no_ranking(&r, 0, 3, 1, 2));
    tarjan_liberar(&r);
    graph_destroy(g);
}

/* Estrela com centro 0 e folhas 1..4: o centro é a raiz da DFS com 4 filhos.
 * Sem ele, 4 pedaços de 1; o maior tem 1, então 3 ficam isolados dele. */
static void estrela(GraphRep rep)
{
    Par a[] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}};
    Graph *g = MONTAR(rep, 5, a);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == 4);
    CHECK(r.num_articulacoes == 1 && no_ranking(&r, 0, 0, 3, 4));
    tarjan_liberar(&r);
    graph_destroy(g);
}

/* Estrela vista de uma folha: centro 2, raiz da DFS na folha 0. O centro não
 * é raiz, e o "resto" (a folha 0) também é um pedaço. */
static void estrela_com_raiz_na_folha(GraphRep rep)
{
    Par a[] = {{0, 2}, {1, 2}, {2, 3}, {2, 4}};
    Graph *g = MONTAR(rep, 5, a);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);
    CHECK(r.num_articulacoes == 1 && no_ranking(&r, 0, 2, 3, 4));
    /* a DFS chega ao 1 vindo do 2: o pai tem índice maior que o filho, e a
     * ponte ainda sai com a ponta menor primeiro */
    CHECK(r.num_pontes == 4 && tem_ponte(&r, 1, 2));
    int ordenadas = 1;
    for (size_t i = 0; i < r.num_pontes; i++)
        ordenadas &= r.pontes[i].u < r.pontes[i].v;
    CHECK(ordenadas);
    tarjan_liberar(&r);
    graph_destroy(g);
}

/* Dois triângulos que compartilham o vértice 2: articulação sem ponte. */
static void gravata_borboleta(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 2}};
    Graph *g = MONTAR(rep, 5, a);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == 0);
    CHECK(r.num_articulacoes == 1 && no_ranking(&r, 0, 2, 2, 2));
    tarjan_liberar(&r);
    graph_destroy(g);
}

/* Ciclo: nenhuma ponte, nenhuma articulação. */
static void ciclo(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    Graph *g = MONTAR(rep, 4, a);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == 0 && r.num_articulacoes == 0);
    tarjan_liberar(&r);
    graph_destroy(g);
}

static void vazio(GraphRep rep)
{
    Graph *g;
    graph_create(0, rep, GRAPH_SYMMETRIC, &g);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == 0 && r.num_articulacoes == 0);
    tarjan_liberar(&r);
    graph_destroy(g);
}

/* Caminho 0-1-...-(n-1): todas as n-1 arestas são pontes e os n-2 vértices
 * internos são articulações. A DFS desce n níveis. O vértice do meio é o de
 * maior impacto: sem ele, sobram duas metades. */
static void caminho_longo(GraphRep rep, size_t n)
{
    Graph *g;
    graph_create(n, rep, GRAPH_SYMMETRIC, &g);
    for (size_t v = 0; v + 1 < n; v++)
        graph_add_edge(g, v, v + 1);
    TarjanResultado r;
    CHECK(tarjan_executar(g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == n - 1 && r.num_articulacoes == n - 2);
    /* n par: os dois do meio separam (n/2 - 1) vértices cada; empate, menor índice primeiro */
    CHECK(no_ranking(&r, 0, n / 2 - 1, n / 2 - 1, 2));
    tarjan_liberar(&r);
    graph_destroy(g);
}

static void rodar(GraphRep rep)
{
    conferencia_g1(rep);
    conferencia_g2(rep);
    estrela(rep);
    estrela_com_raiz_na_folha(rep);
    gravata_borboleta(rep);
    ciclo(rep);
    vazio(rep);
}

int main(void)
{
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);
    caminho_longo(GRAPH_LIST, 100000);
    caminho_longo(GRAPH_MATRIX, 20000);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
