/*
 * Conferência manual automatizada (#13): os grafos desenhados à mão de
 * tests/grafos/, conferidos contra o gabarito de tests/conferencia_manual.md.
 *
 * Cada algoritmo da Fase I ganha aqui o seu caso: BFS (#14), DFS (#15),
 * componentes (#18), ciclos (#19), bipartição (#20) e Tarjan (#21).
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_conferencia && ./bin/test_conferencia
 */
#include "bfs.h"
#include "bipartido.h"
#include "componentes.h"
#include "cycles.h"
#include "dfs.h"
#include "edgelist.h"
#include "graph.h"
#include "idmap.h"
#include "memtrack.h"
#include "tarjan.h"

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

/* DFS, visão direcionada, a partir de 0: com vizinhos em ordem crescente, a
 * ordem de descoberta é 0, 1, ..., 8; 9, 10 e 11 não são alcançados. */
static void dfs_g1(GraphRep rep)
{
    Carregado d = carregar(G1, rep, GRAPH_DIRECTED);
    DfsResultado r;
    CHECK(dfs_executar(d.g, idx(&d, 0), &r) == DFS_OK);
    int ordem = 1;
    for (unsigned long long id = 0; id < 9; id++)
        ordem &= r.descoberta[idx(&d, id)] == id + 1;
    CHECK(ordem);
    CHECK(r.alcancados == 9);
    dfs_liberar(&r);
    descarregar(&d);
}

/* Componentes, visão simetrizada: G1 tem {0..8} e {9, 10, 11}; G2 é um só. */
static void componentes_g1_g2(GraphRep rep)
{
    Carregado s = carregar(G1, rep, GRAPH_SYMMETRIC);
    ComponentesResultado r;
    CHECK(componentes_executar(s.g, &r) == COMPONENTES_OK);
    CHECK(r.num_componentes == 2 && r.tamanho_gigante == 9);
    int juntos = 1;
    for (unsigned long long id = 1; id < 9; id++)
        juntos &= r.componente[idx(&s, id)] == r.componente[idx(&s, 0)];
    juntos &= r.componente[idx(&s, 10)] == r.componente[idx(&s, 9)] &&
              r.componente[idx(&s, 11)] == r.componente[idx(&s, 9)];
    CHECK(juntos && r.componente[idx(&s, 0)] != r.componente[idx(&s, 9)]);
    componentes_liberar(&r);
    descarregar(&s);

    s = carregar(G2, rep, GRAPH_SYMMETRIC);
    CHECK(componentes_executar(s.g, &r) == COMPONENTES_OK);
    CHECK(r.num_componentes == 1 && r.tamanho_gigante == 5);
    componentes_liberar(&r);
    descarregar(&s);
}

/* O ciclo de exemplo tem exatamente os vértices do gabarito (IDs). */
static int ciclo_tem(const Carregado *c, const CiclosResultado *r, const unsigned long long *ids,
                     size_t k)
{
    if (r->tam_ciclo != k)
        return 0;
    for (size_t i = 0; i < k; i++) {
        int achou = 0;
        for (size_t j = 0; j < k; j++)
            achou |= r->ciclo[j] == idx(c, ids[i]);
        if (!achou)
            return 0;
    }
    return 1;
}

/* Ciclos, visão direcionada. G1: retornos 2 -> 0 e 5 -> 3, e o primeiro
 * fecha 0 -> 1 -> 2. G2: o ciclo 0 -> 1 -> 2 -> 3 -> 0, retorno 3 -> 0. */
static void ciclos_g1_g2(GraphRep rep)
{
    Carregado d = carregar(G1, rep, GRAPH_DIRECTED);
    CiclosResultado r;
    CHECK(ciclos_executar(d.g, &r) == CICLOS_OK);
    const unsigned long long tri[] = {0, 1, 2};
    CHECK(r.tem_ciclo && r.retorno == 2 && ciclo_tem(&d, &r, tri, 3));
    ciclos_liberar(&r);
    descarregar(&d);

    d = carregar(G2, rep, GRAPH_DIRECTED);
    CHECK(ciclos_executar(d.g, &r) == CICLOS_OK);
    const unsigned long long quad[] = {0, 1, 2, 3};
    CHECK(r.tem_ciclo && r.retorno == 1 && ciclo_tem(&d, &r, quad, 4));
    ciclos_liberar(&r);
    descarregar(&d);
}

/* Bipartição, visão simetrizada. G1 não é bipartido (triângulos 0-1-2 e
 * 3-4-5); G2 é, com lados {0, 2, 4} e {1, 3}. */
static void bipartido_g1_g2(GraphRep rep)
{
    Carregado s = carregar(G1, rep, GRAPH_SYMMETRIC);
    BipartidoResultado r;
    CHECK(bipartido_executar(s.g, &r) == BIPARTIDO_OK);
    CHECK(!r.bipartido && r.tam_ciclo == 3);
    bipartido_liberar(&r);
    descarregar(&s);

    s = carregar(G2, rep, GRAPH_SYMMETRIC);
    CHECK(bipartido_executar(s.g, &r) == BIPARTIDO_OK);
    unsigned char c0 = r.cor[idx(&s, 0)];
    CHECK(r.bipartido);
    CHECK(r.cor[idx(&s, 2)] == c0 && r.cor[idx(&s, 4)] == c0);
    CHECK(r.cor[idx(&s, 1)] != c0 && r.cor[idx(&s, 3)] != c0);
    bipartido_liberar(&r);
    descarregar(&s);
}

static int ponte_ids(const Carregado *c, const TarjanResultado *r, unsigned long long a,
                     unsigned long long b)
{
    size_t u = idx(c, a), v = idx(c, b);
    if (u > v) {
        size_t t = u;
        u = v;
        v = t;
    }
    for (size_t i = 0; i < r->num_pontes; i++)
        if (r->pontes[i].u == u && r->pontes[i].v == v)
            return 1;
    return 0;
}

/* Tarjan, visão simetrizada. G1: pontes {2,3}, {5,6}, {6,7}, {7,8}, {9,10},
 * {10,11}; articulações 2, 3, 5, 6, 7, 10. G2: ponte {3, 4}; articulação 3. */
static void tarjan_g1_g2(GraphRep rep)
{
    Carregado s = carregar(G1, rep, GRAPH_SYMMETRIC);
    TarjanResultado r;
    CHECK(tarjan_executar(s.g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == 6);
    CHECK(ponte_ids(&s, &r, 2, 3) && ponte_ids(&s, &r, 5, 6) && ponte_ids(&s, &r, 6, 7));
    CHECK(ponte_ids(&s, &r, 7, 8) && ponte_ids(&s, &r, 9, 10) && ponte_ids(&s, &r, 10, 11));
    const unsigned long long art[] = {2, 3, 5, 6, 7, 10};
    int marcadas = r.num_articulacoes == 6;
    for (size_t i = 0; i < 6; i++)
        marcadas &= r.eh_articulacao[idx(&s, art[i])];
    CHECK(marcadas);
    tarjan_liberar(&r);
    descarregar(&s);

    s = carregar(G2, rep, GRAPH_SYMMETRIC);
    CHECK(tarjan_executar(s.g, &r) == TARJAN_OK);
    CHECK(r.num_pontes == 1 && ponte_ids(&s, &r, 3, 4));
    CHECK(r.num_articulacoes == 1 && r.eh_articulacao[idx(&s, 3)]);
    tarjan_liberar(&r);
    descarregar(&s);
}

int main(void)
{
    for (int rep = GRAPH_LIST; rep <= GRAPH_MATRIX; rep++) {
        bfs_g1((GraphRep)rep);
        bfs_g2((GraphRep)rep);
        dfs_g1((GraphRep)rep);
        componentes_g1_g2((GraphRep)rep);
        ciclos_g1_g2((GraphRep)rep);
        bipartido_g1_g2((GraphRep)rep);
        tarjan_g1_g2((GraphRep)rep);
    }

    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
