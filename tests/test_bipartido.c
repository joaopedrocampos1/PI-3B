/*
 * Testes da verificação de bipartição (src/bipartido.c), nas duas
 * representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_bipartido && ./bin/test_bipartido
 */
#include "bipartido.h"
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

/* Toda aresta liga cores diferentes. */
static int coloracao_valida(const Graph *g, const BipartidoResultado *r)
{
    for (size_t u = 0; u < r->n; u++) {
        GraphIter it;
        size_t w;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next(&it, &w))
            if (r->cor[u] == r->cor[w])
                return 0;
    }
    return 1;
}

/* O ciclo devolvido é de fato um ciclo ímpar do grafo: tamanho ímpar (pelo
 * menos 3), vértices distintos e cada um vizinho do seguinte, com o último
 * vizinho do primeiro. Não depende de qual ciclo ímpar foi escolhido. */
static int ciclo_impar_valido(const Graph *g, const BipartidoResultado *r)
{
    size_t k = r->tam_ciclo;
    if (!r->ciclo_impar || k < 3 || k % 2 == 0)
        return 0;
    for (size_t i = 0; i < k; i++) {
        if (r->ciclo_impar[i] >= r->n)
            return 0;
        for (size_t j = 0; j < i; j++)
            if (r->ciclo_impar[i] == r->ciclo_impar[j])
                return 0;
        if (!graph_has_edge(g, r->ciclo_impar[i], r->ciclo_impar[(i + 1) % k]))
            return 0;
    }
    return 1;
}

/* G2 da conferência manual: quadrado 0-1-2-3 com a cauda 3-4. Bipartido, com
 * lados {0, 2, 4} e {1, 3}. */
static void quadrado_com_cauda(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {3, 4}};
    Graph *g = MONTAR(rep, 5, a);
    BipartidoResultado r;
    CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
    CHECK(r.bipartido);
    CHECK(coloracao_valida(g, &r));
    CHECK(r.cor[0] == r.cor[2] && r.cor[2] == r.cor[4]);
    CHECK(r.cor[1] == r.cor[3] && r.cor[0] != r.cor[1]);
    CHECK(r.lado[r.cor[0]] == 3 && r.lado[r.cor[1]] == 2);
    CHECK(r.ciclo_impar == NULL && r.tam_ciclo == 0);
    bipartido_liberar(&r);
    graph_destroy(g);
}

static void triangulo(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}};
    Graph *g = MONTAR(rep, 3, a);
    BipartidoResultado r;
    CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
    CHECK(!r.bipartido);
    CHECK(r.tam_ciclo == 3);
    CHECK(ciclo_impar_valido(g, &r));
    bipartido_liberar(&r);
    graph_destroy(g);
}

/* Pentágono 0-1-2-3-4 com uma cauda 2-5-6: o ciclo ímpar é o pentágono
 * inteiro, e a cauda não entra nele. */
static void pentagono_com_cauda(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}, {2, 5}, {5, 6}};
    Graph *g = MONTAR(rep, 7, a);
    BipartidoResultado r;
    CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
    CHECK(!r.bipartido);
    CHECK(r.tam_ciclo == 5);
    CHECK(ciclo_impar_valido(g, &r));
    int tem_cauda = 0;
    for (size_t i = 0; i < r.tam_ciclo; i++)
        tem_cauda |= r.ciclo_impar[i] == 5 || r.ciclo_impar[i] == 6;
    CHECK(!tem_cauda);
    bipartido_liberar(&r);
    graph_destroy(g);
}

/* Um componente bipartido e outro com triângulo: o grafo não é bipartido,
 * e o ciclo ímpar vem do componente certo, mesmo vindo depois. */
static void um_componente_estraga(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {3, 4}, {4, 5}, {5, 3}};
    Graph *g = MONTAR(rep, 6, a);
    BipartidoResultado r;
    CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
    CHECK(!r.bipartido);
    CHECK(ciclo_impar_valido(g, &r) && r.tam_ciclo == 3);
    bipartido_liberar(&r);
    graph_destroy(g);
}

/* Dois triângulos em componentes separados: o exemplo é sempre o do
 * primeiro conflito encontrado, o do componente do vértice 0, para que a
 * saída seja reproduzível. */
static void exemplo_e_o_primeiro_conflito(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}, {3, 4}, {4, 5}, {5, 3}};
    Graph *g = MONTAR(rep, 6, a);
    BipartidoResultado r;
    CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
    CHECK(!r.bipartido && r.tam_ciclo == 3);
    int do_primeiro = r.tam_ciclo == 3;
    for (size_t i = 0; i < r.tam_ciclo; i++)
        do_primeiro &= r.ciclo_impar[i] <= 2;
    CHECK(do_primeiro);
    bipartido_liberar(&r);
    graph_destroy(g);
}

/* Floresta com vértice isolado: bipartida, todos os componentes coloridos. */
static void floresta(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {4, 5}};   /* 3 isolado */
    Graph *g = MONTAR(rep, 6, a);
    BipartidoResultado r;
    CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
    CHECK(r.bipartido);
    CHECK(coloracao_valida(g, &r));
    CHECK(r.lado[0] + r.lado[1] == 6);
    bipartido_liberar(&r);
    graph_destroy(g);
}

static void vazio(GraphRep rep)
{
    Graph *g;
    graph_create(0, rep, GRAPH_SYMMETRIC, &g);
    BipartidoResultado r;
    CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
    CHECK(r.bipartido && r.lado[0] == 0 && r.lado[1] == 0 && r.tam_ciclo == 0);
    bipartido_liberar(&r);
    graph_destroy(g);
}

/* Ciclos grandes: par (bipartido) e ímpar, cujo ciclo reconstruído é o
 * ciclo inteiro. */
static void ciclos_grandes(GraphRep rep)
{
    for (size_t n = 1000; n <= 1001; n++) {
        Graph *g;
        graph_create(n, rep, GRAPH_SYMMETRIC, &g);
        for (size_t v = 0; v < n; v++)
            graph_add_edge(g, v, (v + 1) % n);
        BipartidoResultado r;
        CHECK(bipartido_executar(g, &r) == BIPARTIDO_OK);
        if (n % 2 == 0) {
            CHECK(r.bipartido && r.lado[0] == n / 2 && r.lado[1] == n / 2);
        } else {
            CHECK(!r.bipartido && r.tam_ciclo == n);
            CHECK(ciclo_impar_valido(g, &r));
        }
        bipartido_liberar(&r);
        graph_destroy(g);
    }
}

static void rodar(GraphRep rep)
{
    quadrado_com_cauda(rep);
    triangulo(rep);
    pentagono_com_cauda(rep);
    um_componente_estraga(rep);
    exemplo_e_o_primeiro_conflito(rep);
    floresta(rep);
    vazio(rep);
    ciclos_grandes(rep);
}

int main(void)
{
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
