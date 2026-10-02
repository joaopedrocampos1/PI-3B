/*
 * Testes do BFS (src/bfs.c), nas duas representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_bfs && ./bin/test_bfs
 */
#include "bfs.h"
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

/* Grafo com n vértices e as arestas dadas, por índice. */
static Graph *montar(GraphRep rep, GraphView view, size_t n, const Par *a, size_t m)
{
    Graph *g;
    if (graph_create(n, rep, view, &g) != GRAPH_OK) {
        fprintf(stderr, "sem memória para o grafo de teste\n");
        exit(2);
    }
    for (size_t i = 0; i < m; i++)
        graph_add_edge(g, a[i].u, a[i].v);
    return g;
}

#define MONTAR(rep, view, n, arestas) montar(rep, view, n, arestas, sizeof arestas / sizeof *arestas)

/* O caminho devolvido é válido: começa em `origem`, termina em `destino`,
 * tem dist + 1 vértices e cada passo é uma aresta do grafo. */
static int caminho_valido(const Graph *g, const BfsResultado *r, size_t origem, size_t destino)
{
    size_t cam[64];
    size_t tam = bfs_caminho(r, destino, cam, 64);
    if (tam != r->dist[destino] + 1 || cam[0] != origem || cam[tam - 1] != destino)
        return 0;
    for (size_t i = 0; i + 1 < tam; i++)
        if (!graph_has_edge(g, cam[i], cam[i + 1]))
            return 0;
    return 1;
}

/* Linha 0-1-2-3-4 (simetrizada): distância é a diferença de índices. */
static void linha(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}};
    Graph *g = MONTAR(rep, GRAPH_SYMMETRIC, 5, a);
    BfsResultado r;
    CHECK(bfs_executar(g, 0, &r) == BFS_OK);
    int ok = 1;
    for (size_t v = 0; v < 5; v++)
        ok &= (r.dist[v] == v);
    CHECK(ok);
    CHECK(r.alcancados == 5);
    CHECK(r.dist_max == 4);
    CHECK(r.dist_media == 2.5);   /* (1+2+3+4)/4 */
    CHECK(r.pred[0] == BFS_NENHUM && r.pred[3] == 2);

    size_t cam[8];
    CHECK(bfs_caminho(&r, 4, cam, 8) == 5);
    CHECK(cam[0] == 0 && cam[1] == 1 && cam[2] == 2 && cam[3] == 3 && cam[4] == 4);
    CHECK(bfs_caminho(&r, 4, cam, 2) == 5);   /* não cabe: só informa o tamanho */
    CHECK(bfs_caminho(&r, 0, cam, 8) == 1 && cam[0] == 0);
    bfs_liberar(&r);

    /* do meio: excentricidade 2 */
    CHECK(bfs_executar(g, 2, &r) == BFS_OK);
    CHECK(r.dist[0] == 2 && r.dist[4] == 2 && r.dist_max == 2);
    bfs_liberar(&r);
    graph_destroy(g);
}

/* Ciclo direcionado 0->1->2->3->4->5->0: só se anda para a frente. */
static void ciclo_direcionado(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 6, a);
    BfsResultado r;
    CHECK(bfs_executar(g, 0, &r) == BFS_OK);
    CHECK(r.dist[1] == 1 && r.dist[5] == 5);   /* de 0 a 5 dá a volta inteira */
    CHECK(r.dist_max == 5 && r.alcancados == 6);
    bfs_liberar(&r);

    CHECK(bfs_executar(g, 3, &r) == BFS_OK);
    CHECK(r.dist[2] == 5 && r.dist[0] == 3);
    CHECK(caminho_valido(g, &r, 3, 2));
    bfs_liberar(&r);
    graph_destroy(g);
}

/* Estrela direcionada: todos seguem o centro 0 (folha -> 0). Do centro não
 * se alcança ninguém; de uma folha, só o centro. */
static void estrela_direcionada(GraphRep rep)
{
    Par a[] = {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 6, a);
    BfsResultado r;
    CHECK(bfs_executar(g, 0, &r) == BFS_OK);
    CHECK(r.alcancados == 1 && r.dist_max == 0 && r.dist_media == 0.0);
    CHECK(r.dist[3] == BFS_INF && !r.visitado[3]);
    size_t cam[8];
    CHECK(bfs_caminho(&r, 3, cam, 8) == 0);   /* inalcançável */
    bfs_liberar(&r);

    CHECK(bfs_executar(g, 4, &r) == BFS_OK);
    CHECK(r.alcancados == 2 && r.dist[0] == 1 && r.dist[1] == BFS_INF);
    bfs_liberar(&r);
    graph_destroy(g);

    /* simetrizada: do centro, todas as folhas a 1; entre folhas, 2 */
    g = MONTAR(rep, GRAPH_SYMMETRIC, 6, a);
    CHECK(bfs_executar(g, 1, &r) == BFS_OK);
    CHECK(r.dist[0] == 1 && r.dist[5] == 2 && r.dist_max == 2);
    CHECK(caminho_valido(g, &r, 1, 5));
    bfs_liberar(&r);
    graph_destroy(g);
}

/* Dois componentes, {0,1,2} e {3,4}, mais o vértice isolado 5. */
static void desconexo(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {3, 4}};
    Graph *g = MONTAR(rep, GRAPH_SYMMETRIC, 6, a);
    BfsResultado r;
    CHECK(bfs_executar(g, 0, &r) == BFS_OK);
    CHECK(r.alcancados == 3);
    CHECK(r.dist[3] == BFS_INF && r.dist[4] == BFS_INF && r.dist[5] == BFS_INF);
    bfs_liberar(&r);

    CHECK(bfs_executar(g, 5, &r) == BFS_OK);   /* isolado: só ele mesmo */
    CHECK(r.alcancados == 1 && r.dist[5] == 0);
    bfs_liberar(&r);
    graph_destroy(g);
}

/* Bipartido completo K(3,3): lados {0,1,2} e {3,4,5}. Mesmo lado a 2,
 * lado oposto a 1. */
static void bipartido(GraphRep rep)
{
    Par a[9];
    size_t m = 0;
    for (size_t u = 0; u < 3; u++)
        for (size_t v = 3; v < 6; v++)
            a[m++] = (Par){u, v};
    Graph *g = montar(rep, GRAPH_SYMMETRIC, 6, a, m);
    BfsResultado r;
    CHECK(bfs_executar(g, 0, &r) == BFS_OK);
    CHECK(r.dist[1] == 2 && r.dist[2] == 2);
    CHECK(r.dist[3] == 1 && r.dist[4] == 1 && r.dist[5] == 1);
    /* vizinhos em ordem crescente: 1 é alcançado pelo primeiro vizinho, 3 */
    CHECK(r.pred[1] == 3);
    bfs_liberar(&r);
    graph_destroy(g);
}

/* Várias origens: a distância é até a mais próxima. */
static void multi_origem(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}};
    Graph *g = MONTAR(rep, GRAPH_SYMMETRIC, 7, a);
    size_t origens[] = {0, 6, 6};   /* repetida de propósito */
    BfsResultado r;
    CHECK(bfs_executar_multi(g, origens, 3, &r) == BFS_OK);
    CHECK(r.dist[0] == 0 && r.dist[6] == 0);
    CHECK(r.dist[1] == 1 && r.dist[5] == 1 && r.dist[3] == 3);
    CHECK(r.dist_max == 3 && r.alcancados == 7);
    CHECK(r.pred[0] == BFS_NENHUM && r.pred[6] == BFS_NENHUM);
    size_t cam[8];
    CHECK(bfs_caminho(&r, 5, cam, 8) == 2 && cam[0] == 6 && cam[1] == 5);
    bfs_liberar(&r);
    graph_destroy(g);
}

/* DoD do #14: reconstrói o caminho entre dois vértices quaisquer.
 * Grafo direcionado pseudoaleatório com 30 vértices, conferido contra
 * Floyd-Warshall, em todos os pares. */
static void todos_os_pares(GraphRep rep)
{
    enum { N = 30, M = 70 };
    Par a[M];
    unsigned long long x = 12345;
    for (size_t i = 0; i < M; i++) {
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        a[i].u = (size_t)(x >> 33) % N;
        x = x * 6364136223846793005ULL + 1442695040888963407ULL;
        a[i].v = (size_t)(x >> 33) % N;
    }
    Graph *g = montar(rep, GRAPH_DIRECTED, N, a, M);

    static size_t fw[N][N];
    for (size_t u = 0; u < N; u++)
        for (size_t v = 0; v < N; v++)
            fw[u][v] = (u == v) ? 0 : graph_has_edge(g, u, v) ? 1 : BFS_INF;
    for (size_t k = 0; k < N; k++)
        for (size_t u = 0; u < N; u++)
            for (size_t v = 0; v < N; v++)
                if (fw[u][k] != BFS_INF && fw[k][v] != BFS_INF && fw[u][k] + fw[k][v] < fw[u][v])
                    fw[u][v] = fw[u][k] + fw[k][v];

    int dist_ok = 1, caminhos_ok = 1;
    for (size_t u = 0; u < N; u++) {
        BfsResultado r;
        CHECK(bfs_executar(g, u, &r) == BFS_OK);
        for (size_t v = 0; v < N; v++) {
            dist_ok &= (r.dist[v] == fw[u][v]);
            if (r.visitado[v])
                caminhos_ok &= caminho_valido(g, &r, u, v);
        }
        bfs_liberar(&r);
    }
    CHECK(dist_ok);
    CHECK(caminhos_ok);
    graph_destroy(g);
}

/* Lista e matriz produzem exatamente o mesmo resultado, predecessores
 * incluídos (#12). */
static void lista_igual_matriz(void)
{
    Par a[] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}, {4, 0}, {2, 5}, {5, 6}, {6, 3}};
    Graph *l = MONTAR(GRAPH_LIST, GRAPH_DIRECTED, 7, a);
    Graph *m = MONTAR(GRAPH_MATRIX, GRAPH_DIRECTED, 7, a);
    int iguais = 1;
    for (size_t s = 0; s < 7; s++) {
        BfsResultado rl, rm;
        CHECK(bfs_executar(l, s, &rl) == BFS_OK);
        CHECK(bfs_executar(m, s, &rm) == BFS_OK);
        iguais &= (rl.alcancados == rm.alcancados && rl.dist_max == rm.dist_max);
        for (size_t v = 0; v < 7; v++)
            iguais &= (rl.dist[v] == rm.dist[v] && rl.pred[v] == rm.pred[v]);
        bfs_liberar(&rl);
        bfs_liberar(&rm);
    }
    CHECK(iguais);
    graph_destroy(l);
    graph_destroy(m);
}

static void erros(void)
{
    Graph *g;
    CHECK(graph_create(3, GRAPH_LIST, GRAPH_DIRECTED, &g) == GRAPH_OK);
    BfsResultado r;
    CHECK(bfs_executar(g, 3, &r) == BFS_ERRO_ORIGEM);
    CHECK(r.dist == NULL);
    bfs_liberar(&r);   /* resultado zerado: não pode quebrar */
    CHECK(bfs_executar_multi(g, NULL, 0, &r) == BFS_ERRO_ORIGEM);
    graph_destroy(g);
}

int main(void)
{
    for (int rep = GRAPH_LIST; rep <= GRAPH_MATRIX; rep++) {
        linha((GraphRep)rep);
        ciclo_direcionado((GraphRep)rep);
        estrela_direcionada((GraphRep)rep);
        desconexo((GraphRep)rep);
        bipartido((GraphRep)rep);
        multi_origem((GraphRep)rep);
        todos_os_pares((GraphRep)rep);
    }
    lista_igual_matriz();
    erros();

    /* tudo o que o BFS e os grafos alocaram foi devolvido */
    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
