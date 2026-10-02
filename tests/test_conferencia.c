/*
 * Conferência manual automatizada (#13): os grafos desenhados à mão de
 * tests/grafos/, conferidos contra o gabarito de tests/conferencia_manual.md.
 *
 * Cada algoritmo da Fase I ganha aqui o seu caso quando for implementado.
 * Por enquanto: BFS (#14).
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_conferencia && ./bin/test_conferencia
 */
#include "bfs.h"
#include "edgelist.h"
#include "graph.h"
#include "idmap.h"
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

#define G1 "tests/grafos/conferencia_g1.txt"
#define G2 "tests/grafos/conferencia_g2.txt"
#define INF BFS_INF

typedef struct {
    EdgeList el;
    IdMap ids;
    Graph *g;
} Carregado;

static Carregado carregar(const char *caminho, GraphRep rep, GraphView view)
{
    Carregado c;
    if (edgelist_ler(caminho, &c.el) != EDGELIST_OK) {
        fprintf(stderr, "não foi possível ler %s (rode a partir da raiz do repositório)\n", caminho);
        exit(2);
    }
    idmap_iniciar(&c.ids);
    if (graph_build(&c.el, &c.ids, rep, view, &c.g) != GRAPH_OK) {
        fprintf(stderr, "sem memória para montar %s\n", caminho);
        exit(2);
    }
    return c;
}

static void descarregar(Carregado *c)
{
    graph_destroy(c->g);
    idmap_liberar(&c->ids);
    edgelist_liberar(&c->el);
}

/* Índice interno do ID do gabarito. */
static size_t idx(const Carregado *c, unsigned long long id)
{
    size_t i;
    if (!idmap_buscar(&c->ids, id, &i)) {
        fprintf(stderr, "ID %llu não está no grafo\n", id);
        exit(2);
    }
    return i;
}

/* BFS a partir de `origem` confere com as distâncias do gabarito (indexadas
 * pelo ID), o número de alcançados, a excentricidade e a distância média. */
static void conferir_bfs(const Carregado *c, unsigned long long origem, const size_t *dist,
                         size_t n, size_t alcancados, size_t ecc, double media)
{
    BfsResultado r;
    CHECK(bfs_executar(c->g, idx(c, origem), &r) == BFS_OK);
    int ok = 1;
    for (unsigned long long id = 0; id < n; id++)
        ok &= (r.dist[idx(c, id)] == dist[id]);
    if (!ok)
        fprintf(stderr, "  distâncias erradas a partir de %llu (%s, %s)\n", origem,
                graph_rep_name(graph_rep(c->g)), graph_view_name(graph_view(c->g)));
    CHECK(ok);
    CHECK(r.alcancados == alcancados);
    CHECK(r.dist_max == ecc);
    CHECK(r.dist_media == media);
    bfs_liberar(&r);
}

/* Caminho de `de` até `ate` igual ao do gabarito (IDs). */
static void conferir_caminho(const Carregado *c, unsigned long long de, unsigned long long ate,
                             const unsigned long long *esperado, size_t tam)
{
    BfsResultado r;
    CHECK(bfs_executar(c->g, idx(c, de), &r) == BFS_OK);
    size_t cam[16];
    CHECK(bfs_caminho(&r, idx(c, ate), cam, 16) == tam);
    int ok = 1;
    for (size_t i = 0; i < tam; i++)
        ok &= (cam[i] == idx(c, esperado[i]));
    CHECK(ok);
    bfs_liberar(&r);
}

static void bfs_g1(GraphRep rep)
{
    Carregado d = carregar(G1, rep, GRAPH_DIRECTED);
    CHECK(graph_num_vertices(d.g) == 12 && graph_num_edges(d.g) == 12);

    const size_t de0[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, INF, INF, INF};
    const size_t de2[12] = {1, 2, 0, 1, 2, 3, 4, 5, 6, INF, INF, INF};
    const size_t de6[12] = {INF, INF, INF, INF, INF, INF, 0, 1, 2, INF, INF, INF};
    const size_t de8[12] = {INF, INF, INF, INF, INF, INF, INF, INF, 0, INF, INF, INF};
    const size_t de9[12] = {INF, INF, INF, INF, INF, INF, INF, INF, INF, 0, 1, 2};
    conferir_bfs(&d, 0, de0, 12, 9, 8, 4.5);
    conferir_bfs(&d, 2, de2, 12, 9, 6, 3.0);
    conferir_bfs(&d, 6, de6, 12, 3, 2, 1.5);
    conferir_bfs(&d, 8, de8, 12, 1, 0, 0.0);
    conferir_bfs(&d, 9, de9, 12, 3, 2, 1.5);
    const unsigned long long c08[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    conferir_caminho(&d, 0, 8, c08, 9);
    descarregar(&d);

    Carregado s = carregar(G1, rep, GRAPH_SYMMETRIC);
    const size_t sim0[12] = {0, 1, 1, 2, 3, 3, 4, 5, 6, INF, INF, INF};
    conferir_bfs(&s, 0, sim0, 12, 9, 6, 3.125);
    const unsigned long long s08[] = {0, 2, 3, 5, 6, 7, 8};
    conferir_caminho(&s, 0, 8, s08, 7);

    /* predecessores da tabela do gabarito */
    BfsResultado r;
    CHECK(bfs_executar(s.g, idx(&s, 0), &r) == BFS_OK);
    const unsigned long long pred[8][2] = {{1, 0}, {2, 0}, {3, 2}, {4, 3}, {5, 3},
                                           {6, 5}, {7, 6}, {8, 7}};
    int ok = r.pred[idx(&s, 0)] == BFS_NENHUM;
    for (size_t i = 0; i < 8; i++)
        ok &= (r.pred[idx(&s, pred[i][0])] == idx(&s, pred[i][1]));
    CHECK(ok);
    bfs_liberar(&r);
    descarregar(&s);
}

static void bfs_g2(GraphRep rep)
{
    Carregado d = carregar(G2, rep, GRAPH_DIRECTED);
    const size_t dir0[5] = {0, 1, 2, 3, 4};
    conferir_bfs(&d, 0, dir0, 5, 5, 4, 2.5);
    descarregar(&d);

    Carregado s = carregar(G2, rep, GRAPH_SYMMETRIC);
    const size_t sim0[5] = {0, 1, 2, 1, 2};
    conferir_bfs(&s, 0, sim0, 5, 5, 2, 1.5);
    descarregar(&s);
}

int main(void)
{
    for (int rep = GRAPH_LIST; rep <= GRAPH_MATRIX; rep++) {
        bfs_g1((GraphRep)rep);
        bfs_g2((GraphRep)rep);
    }

    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
