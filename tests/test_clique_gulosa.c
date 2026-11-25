/*
 * Testes da heurística gulosa da Clique Máxima (src/clique_gulosa.c), nas
 * duas representações. A referência para o tamanho máximo é a busca exata
 * (#67), já conferida com um Bron–Kerbosch independente em test_clique.c.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_clique_gulosa && ./bin/test_clique_gulosa
 */
#include "clique.h"
#include "edgelist.h"
#include "idmap.h"
#include "memtrack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    if (graph_create(n, rep, GRAPH_RECIPROCAL, &g) != GRAPH_OK)
        exit(2);
    for (size_t i = 0; i < m; i++)
        graph_add_edge(g, a[i].u, a[i].v);
    return g;
}

#define MONTAR(rep, n, arestas) montar(rep, n, arestas, sizeof arestas / sizeof *arestas)

/* Índices válidos, em ordem estritamente crescente e todos vizinhos entre si. */
static int clique_valida(const Graph *g, const CliqueResultado *r)
{
    if (r->tam > 0 && !r->clique)
        return 0;
    for (size_t i = 0; i < r->tam; i++) {
        if (r->clique[i] >= r->n || (i > 0 && r->clique[i - 1] >= r->clique[i]))
            return 0;
        for (size_t j = 0; j < i; j++)
            if (!graph_has_edge(g, r->clique[i], r->clique[j]))
                return 0;
    }
    return 1;
}

static int clique_e(const CliqueResultado *r, const size_t *esperada, size_t k)
{
    return r->tam == k && (k == 0 || memcmp(r->clique, esperada, k * sizeof *esperada) == 0);
}

/* Grafo completo: a gulosa acha a clique inteira a partir de um início só, e
 * acrescenta exatamente os 5 vértices. */
static void completo(GraphRep rep)
{
    Graph *g;
    graph_create(5, rep, GRAPH_RECIPROCAL, &g);
    for (size_t u = 0; u < 5; u++)
        for (size_t v = u + 1; v < 5; v++)
            graph_add_edge(g, u, v);
    CliqueResultado r;
    CHECK(clique_gulosa(g, 1, &r) == CLIQUE_OK);
    const size_t todos[] = {0, 1, 2, 3, 4};
    CHECK(clique_e(&r, todos, 5));
    CHECK(r.nos == 5 && r.exato == 0 && r.n == 5);
    clique_liberar(&r);
    graph_destroy(g);
}

/* Casos de borda: sem vértices, sem arestas (a clique é um vértice só, o de
 * menor índice) e um ciclo de 5 (clique máxima de 2). */
static void bordas(GraphRep rep)
{
    Graph *g;
    CliqueResultado r;
    graph_create(0, rep, GRAPH_RECIPROCAL, &g);
    CHECK(clique_gulosa(g, 0, &r) == CLIQUE_OK);
    CHECK(r.n == 0 && r.tam == 0 && r.clique == NULL && r.exato == 0);
    clique_liberar(&r);
    graph_destroy(g);

    graph_create(4, rep, GRAPH_RECIPROCAL, &g);
    CHECK(clique_gulosa(g, 0, &r) == CLIQUE_OK);
    const size_t zero[] = {0};
    CHECK(clique_e(&r, zero, 1));
    clique_liberar(&r);
    graph_destroy(g);

    const Par ciclo[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}};
    g = MONTAR(rep, 5, ciclo);
    CHECK(clique_gulosa(g, 0, &r) == CLIQUE_OK);
    const size_t par[] = {0, 1};
    CHECK(clique_e(&r, par, 2));
    clique_liberar(&r);
    graph_destroy(g);
}

/* Erro esperado 1: o vértice de maior grau não está na clique máxima. O hub 0
 * tem 6 folhas soltas; a clique máxima é o K4 {7, 8, 9, 10}, de grau 3.
 * Com um início, a gulosa parte do hub e acha {0, 1}; com dois, o segundo
 * início (7) acha o K4. */
static void hub_fora_da_clique(GraphRep rep)
{
    const Par a[] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6},
                     {7, 8}, {7, 9}, {7, 10}, {8, 9}, {8, 10}, {9, 10}};
    Graph *g = MONTAR(rep, 11, a);
    CliqueResultado r;
    CHECK(clique_gulosa(g, 1, &r) == CLIQUE_OK);
    const size_t errada[] = {0, 1};
    CHECK(clique_e(&r, errada, 2) && r.nos == 2);
    clique_liberar(&r);

    CHECK(clique_gulosa(g, 2, &r) == CLIQUE_OK);
    const size_t k4[] = {7, 8, 9, 10};
    CHECK(clique_e(&r, k4, 4) && r.nos == 6);
    clique_liberar(&r);
    graph_destroy(g);
}

/* Erro esperado 2: o candidato de maior grau desvia a clique. {0, 1, 2, 3} é
 * um K4, e 4 liga 0 a cinco folhas (grau 6, o maior). Os inícios, por grau:
 * 4 (6), 0 (4), 1, 2, 3 (3 cada).
 *   - de 4: entra 0 (grau 4) e não sobra candidato: {0, 4};
 *   - de 0: entra 4 (grau 6, maior que o de 1, 2 e 3) e para: {0, 4};
 *   - de 1: os candidatos são 0, 2, 3; entra 0, depois 2, depois 3: o K4.
 * Então 1 e 2 inícios erram, e 3 acertam. */
static void vizinho_que_desvia(GraphRep rep)
{
    const Par a[] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3},
                     {0, 4}, {4, 5}, {4, 6}, {4, 7}, {4, 8}, {4, 9}};
    Graph *g = MONTAR(rep, 10, a);
    CliqueResultado r;
    const size_t errada[] = {0, 4};
    const size_t k4[] = {0, 1, 2, 3};
    for (size_t k = 1; k <= 2; k++) {
        CHECK(clique_gulosa(g, k, &r) == CLIQUE_OK);
        CHECK(clique_e(&r, errada, 2));
        clique_liberar(&r);
    }
    CHECK(clique_gulosa(g, 3, &r) == CLIQUE_OK);
    CHECK(clique_e(&r, k4, 4));
    clique_liberar(&r);

    CliqueResultado e;
    CHECK(clique_exata(g, 0, &e) == CLIQUE_OK && e.tam == 4);
    clique_liberar(&e);
    graph_destroy(g);
}

/* Inícios demais (mais que V) valem o mesmo que todos (0). */
static void inicios_alem_de_v(GraphRep rep)
{
    const Par a[] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}};
    Graph *g = MONTAR(rep, 4, a);
    CliqueResultado r, s;
    CHECK(clique_gulosa(g, 0, &r) == CLIQUE_OK);
    CHECK(clique_gulosa(g, 1000, &s) == CLIQUE_OK);
    CHECK(r.tam == 3 && s.tam == 3 && r.nos == s.nos);
    clique_liberar(&r);
    clique_liberar(&s);
    graph_destroy(g);
}

static unsigned long proximo(unsigned long *s)
{
    *s = (*s * 1103515245UL + 12345UL) & 0x7fffffffUL;
    return *s;
}

/* Grafos aleatórios de várias densidades: a gulosa devolve sempre uma clique
 * válida, nunca maior que a máxima, e mais inícios nunca pioram o resultado;
 * lista e matriz dão a mesma clique. */
static void aleatorios(void)
{
    unsigned long s = 68;
    int validas = 1, limitadas = 1, monotonas = 1, iguais = 1;
    for (int caso = 0; caso < 60; caso++) {
        size_t n = 5 + proximo(&s) % 40;
        unsigned long dens = 10 + proximo(&s) % 80;   /* % de pares ligados */
        Graph *l, *m;
        graph_create(n, GRAPH_LIST, GRAPH_RECIPROCAL, &l);
        graph_create(n, GRAPH_MATRIX, GRAPH_RECIPROCAL, &m);
        for (size_t u = 0; u < n; u++)
            for (size_t v = u + 1; v < n; v++)
                if (proximo(&s) % 100 < dens) {
                    graph_add_edge(l, u, v);
                    graph_add_edge(m, u, v);
                }
        CliqueResultado e;
        clique_exata(l, 0, &e);
        size_t anterior = 0;
        for (size_t k = 1; k <= n; k++) {
            CliqueResultado rl, rm;
            clique_gulosa(l, k, &rl);
            clique_gulosa(m, k, &rm);
            validas &= clique_valida(l, &rl);
            limitadas &= rl.tam >= 1 && rl.tam <= e.tam;
            monotonas &= rl.tam >= anterior;
            iguais &= clique_e(&rm, rl.clique, rl.tam) && rm.nos == rl.nos;
            anterior = rl.tam;
            clique_liberar(&rl);
            clique_liberar(&rm);
        }
        clique_liberar(&e);
        graph_destroy(l);
        graph_destroy(m);
    }
    CHECK(validas);
    CHECK(limitadas);
    CHECK(monotonas);
    CHECK(iguais);
}

/* Amostras reais de 100 vértices, na visão recíproca: clique válida, nunca
 * maior que a exata, e a mesma com lista e matriz. */
static void amostras(void)
{
    const char *arquivos[] = {"data/samples/bfs_307642294_n100.txt",
                              "data/samples/bfs_144319796_n100.txt",
                              "data/samples/bfs_9487272_n100.txt"};
    for (size_t i = 0; i < 3; i++) {
        EdgeList el;
        if (edgelist_ler(arquivos[i], &el) != EDGELIST_OK) {
            fprintf(stderr, "não foi possível ler %s (rode a partir da raiz)\n", arquivos[i]);
            exit(2);
        }
        IdMap ids;
        idmap_iniciar(&ids);
        Graph *l, *m;
        graph_build(&el, &ids, GRAPH_LIST, GRAPH_RECIPROCAL, &l);
        graph_build(&el, &ids, GRAPH_MATRIX, GRAPH_RECIPROCAL, &m);
        CliqueResultado e, rl, rm;
        CHECK(clique_exata(l, 0, &e) == CLIQUE_OK && e.exato);
        CHECK(clique_gulosa(l, 10, &rl) == CLIQUE_OK);
        CHECK(clique_gulosa(m, 10, &rm) == CLIQUE_OK);
        CHECK(clique_valida(l, &rl) && rl.tam <= e.tam && rl.tam >= 2);
        CHECK(clique_e(&rm, rl.clique, rl.tam));
        clique_liberar(&e);
        clique_liberar(&rl);
        clique_liberar(&rm);
        graph_destroy(l);
        graph_destroy(m);
        idmap_liberar(&ids);
        edgelist_liberar(&el);
    }
}

int main(void)
{
    for (GraphRep rep = GRAPH_LIST; rep <= GRAPH_MATRIX; rep++) {
        completo(rep);
        bordas(rep);
        hub_fora_da_clique(rep);
        vizinho_que_desvia(rep);
        inicios_alem_de_v(rep);
    }
    aleatorios();
    amostras();
    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
