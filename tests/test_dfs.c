/*
 * Testes da DFS iterativa (src/dfs.c), nas duas representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_dfs && ./bin/test_dfs
 */
#include "dfs.h"
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

/* Teorema dos parênteses: tempos distintos de 1 a 2V, descoberta antes da
 * finalização, e intervalos de dois vértices disjuntos ou um dentro do outro. */
static int parenteses(const DfsResultado *r)
{
    size_t n = r->n;
    unsigned char *usado = calloc(2 * n + 1, 1);
    int ok = usado != NULL;
    for (size_t v = 0; ok && v < n; v++) {
        size_t d = r->descoberta[v], f = r->finalizacao[v];
        ok = d >= 1 && f <= 2 * n && d < f && !usado[d] && !usado[f];
        if (ok)
            usado[d] = usado[f] = 1;
    }
    for (size_t u = 0; ok && u < n; u++)
        for (size_t v = 0; ok && v < n; v++) {
            size_t du = r->descoberta[u], fu = r->finalizacao[u];
            size_t dv = r->descoberta[v], fv = r->finalizacao[v];
            int disjuntos = fu < dv || fv < du;
            int u_contem_v = du < dv && fv < fu;
            int v_contem_u = dv < du && fu < fv;
            ok = u == v || disjuntos || u_contem_v || v_contem_u;
        }
    free(usado);
    return ok;
}

/* G1 da conferência manual, a partir de 0. Tempos à mão, evento a evento:
 * desce 0, 1, 2, 3, 4, 5, 6, 7, 8 (2 -> 0 e 5 -> 3 voltam a já descobertos),
 * finaliza de 8 a 0, e a segunda árvore desce 9, 10, 11. */
static void conferencia_g1(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 5},
               {5, 3}, {5, 6}, {6, 7}, {7, 8}, {9, 10}, {10, 11}};
    Graph *g = MONTAR(rep, 12, a);
    DfsResultado r;
    CHECK(dfs_executar(g, 0, &r) == DFS_OK);
    static const size_t d[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 19, 20, 21};
    static const size_t f[] = {18, 17, 16, 15, 14, 13, 12, 11, 10, 24, 23, 22};
    int tempos = 1;
    for (size_t v = 0; v < 12; v++)
        tempos &= r.descoberta[v] == d[v] && r.finalizacao[v] == f[v];
    CHECK(tempos);
    CHECK(r.arvores == 2 && r.alcancados == 9);
    CHECK(parenteses(&r));
    dfs_liberar(&r);
    graph_destroy(g);
}

/* A partir de 9, a primeira árvore é {9, 10, 11}; o resto vem depois, a
 * partir de 0. */
static void origem_no_meio(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 5},
               {5, 3}, {5, 6}, {6, 7}, {7, 8}, {9, 10}, {10, 11}};
    Graph *g = MONTAR(rep, 12, a);
    DfsResultado r;
    CHECK(dfs_executar(g, 9, &r) == DFS_OK);
    CHECK(r.descoberta[9] == 1 && r.finalizacao[9] == 6);
    CHECK(r.descoberta[0] == 7);
    CHECK(r.arvores == 2 && r.alcancados == 3);
    CHECK(parenteses(&r));
    dfs_liberar(&r);
    graph_destroy(g);
}

/* Sem arestas: cada vértice é uma árvore, com intervalo de tamanho 1. */
static void sem_arestas(GraphRep rep)
{
    Graph *g;
    graph_create(4, rep, GRAPH_DIRECTED, &g);
    DfsResultado r;
    CHECK(dfs_executar(g, 2, &r) == DFS_OK);
    CHECK(r.arvores == 4 && r.alcancados == 1);
    CHECK(r.descoberta[2] == 1 && r.finalizacao[2] == 2);
    CHECK(r.descoberta[0] == 3 && r.finalizacao[3] == 8);
    dfs_liberar(&r);
    graph_destroy(g);
}

static void origem_invalida(GraphRep rep)
{
    Graph *g;
    graph_create(3, rep, GRAPH_DIRECTED, &g);
    DfsResultado r;
    CHECK(dfs_executar(g, 3, &r) == DFS_ERRO_ORIGEM);
    CHECK(r.descoberta == NULL && r.arvores == 0);
    graph_destroy(g);

    graph_create(0, rep, GRAPH_DIRECTED, &g);
    CHECK(dfs_executar(g, 0, &r) == DFS_ERRO_ORIGEM);
    graph_destroy(g);
}

/* Caminho 0 -> 1 -> ... -> n-1: a DFS desce n níveis. */
static void caminho_longo(GraphRep rep, size_t n)
{
    Graph *g;
    graph_create(n, rep, GRAPH_DIRECTED, &g);
    for (size_t v = 0; v + 1 < n; v++)
        graph_add_edge(g, v, v + 1);
    DfsResultado r;
    CHECK(dfs_executar(g, 0, &r) == DFS_OK);
    CHECK(r.arvores == 1 && r.alcancados == n);
    CHECK(r.descoberta[n - 1] == n && r.finalizacao[n - 1] == n + 1 && r.finalizacao[0] == 2 * n);
    dfs_liberar(&r);
    graph_destroy(g);
}

static void rodar(GraphRep rep)
{
    conferencia_g1(rep);
    origem_no_meio(rep);
    sem_arestas(rep);
    origem_invalida(rep);
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
