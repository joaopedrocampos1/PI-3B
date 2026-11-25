/*
 * Testes do Dijkstra (src/dijkstra.c), nas duas representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_dijkstra && ./bin/test_dijkstra
 */
#include "bfs.h"
#include "dijkstra.h"
#include "edgelist.h"
#include "memtrack.h"

#include <math.h>
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
    float peso;
} Aresta3;

static Graph *montar(GraphRep rep, GraphView view, size_t n, const Aresta3 *a, size_t m)
{
    Graph *g;
    if (graph_create_weighted(n, rep, view, &g) != GRAPH_OK)
        exit(2);
    for (size_t i = 0; i < m; i++)
        if (graph_add_weighted_edge(g, a[i].u, a[i].v, a[i].peso) != GRAPH_OK)
            exit(2);
    return g;
}

#define MONTAR(rep, view, n, arestas) montar(rep, view, n, arestas, sizeof arestas / sizeof *arestas)

/* O caminho devolvido existe no grafo, vai da origem ao destino e custa
 * exatamente a distância calculada. */
static int caminho_coerente(const Graph *g, const DijkstraResultado *r, size_t destino)
{
    size_t tam = dijkstra_caminho(r, destino, NULL, 0);
    if (tam == 0)
        return isinf(r->dist[destino]);
    size_t *c = malloc(tam * sizeof *c);
    int ok = dijkstra_caminho(r, destino, c, tam) == tam && c[0] == r->origem &&
             c[tam - 1] == destino;
    double custo = 0.0;
    for (size_t i = 1; ok && i < tam; i++) {
        ok = graph_has_edge(g, c[i - 1], c[i]);
        custo += graph_edge_weight(g, c[i - 1], c[i]);
    }
    free(c);
    return ok && custo == r->dist[destino];
}

/* Exemplo clássico, com custos calculados à mão a partir de 0:
 *   0 -> 1 (7)   0 -> 2 (9)   0 -> 5 (14)   1 -> 2 (10)   1 -> 3 (15)
 *   2 -> 3 (11)  2 -> 5 (2)   3 -> 4 (6)    5 -> 4 (9)
 * O melhor até 4 é 0 -> 2 -> 5 -> 4 (20), e não 0 -> 2 -> 3 -> 4 (26);
 * até 5, 0 -> 2 -> 5 (11) ganha da aresta direta (14). */
static void caminho_calculado_a_mao(GraphRep rep)
{
    const Aresta3 a[] = {{0, 1, 7}, {0, 2, 9}, {0, 5, 14}, {1, 2, 10}, {1, 3, 15},
                         {2, 3, 11}, {2, 5, 2}, {3, 4, 6}, {5, 4, 9}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 6, a);
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    const double esperado[] = {0, 7, 9, 20, 20, 11};
    for (size_t v = 0; v < 6; v++)
        CHECK(r.dist[v] == esperado[v]);
    CHECK(r.alcancados == 6);
    CHECK(r.pred[0] == DIJKSTRA_NENHUM && r.pred[5] == 2 && r.pred[4] == 5);

    size_t c[6];
    CHECK(dijkstra_caminho(&r, 4, c, 6) == 4);
    CHECK(c[0] == 0 && c[1] == 2 && c[2] == 5 && c[3] == 4);
    CHECK(dijkstra_caminho(&r, 0, c, 6) == 1 && c[0] == 0);
    for (size_t v = 0; v < 6; v++)
        CHECK(caminho_coerente(g, &r, v));
    dijkstra_liberar(&r);
    graph_destroy(g);
}

/* Pesos fracionários e peso zero: atrito acumulado em double. O caminho de
 * mais arestas ganha quando custa menos. */
static void pesos_fracionarios(GraphRep rep)
{
    const Aresta3 a[] = {{0, 3, 1.0f}, {0, 1, 0.25f}, {1, 2, 0.0f}, {2, 3, 0.5f}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 4, a);
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    CHECK(r.dist[1] == 0.25 && r.dist[2] == 0.25 && r.dist[3] == 0.75);
    CHECK(dijkstra_caminho(&r, 3, NULL, 0) == 4);
    CHECK(r.pred[3] == 2);
    dijkstra_liberar(&r);
    graph_destroy(g);
}

/* Empate de custo: fica o caminho cujo penúltimo vértice saiu primeiro da
 * fila. No primeiro grafo, 1 e 2 saem com o mesmo custo e o menor índice
 * sai antes; no segundo, 2 sai antes porque custa menos. */
static void empate(GraphRep rep)
{
    const Aresta3 a[] = {{0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 3, 1}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 4, a);
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    CHECK(r.dist[3] == 2 && r.pred[3] == 1);
    dijkstra_liberar(&r);
    graph_destroy(g);

    const Aresta3 b[] = {{0, 1, 1.0f}, {0, 2, 0.5f}, {1, 3, 1.0f}, {2, 3, 1.5f}};
    g = MONTAR(rep, GRAPH_DIRECTED, 4, b);
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    CHECK(r.dist[3] == 2 && r.pred[3] == 2);
    dijkstra_liberar(&r);
    graph_destroy(g);
}

/* Destino inalcançável: distância infinita, sem predecessor e sem caminho.
 * O sentido das arestas importa: 3 alcança 0, mas 0 não alcança 3. */
static void inalcancavel(GraphRep rep)
{
    const Aresta3 a[] = {{0, 1, 2}, {3, 0, 1}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 5, a);
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    CHECK(r.alcancados == 2);
    CHECK(isinf(r.dist[3]) && r.pred[3] == DIJKSTRA_NENHUM);
    CHECK(isinf(r.dist[4]) && r.pred[4] == DIJKSTRA_NENHUM);
    size_t c[5];
    CHECK(dijkstra_caminho(&r, 3, c, 5) == 0);
    CHECK(dijkstra_caminho(&r, 99, c, 5) == 0);   /* fora do grafo */
    dijkstra_liberar(&r);
    graph_destroy(g);
}

/* Nas visões sem sentido, a aresta vale nos dois sentidos. */
static void sem_sentido(GraphRep rep)
{
    const Aresta3 a[] = {{1, 0, 2}, {2, 1, 3}};
    Graph *g = MONTAR(rep, GRAPH_SYMMETRIC, 3, a);
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    CHECK(r.dist[2] == 5 && r.pred[2] == 1);
    dijkstra_liberar(&r);
    graph_destroy(g);
}

/* Caminho maior que o espaço dado: nada é gravado, mas o tamanho volta. */
static void caminho_sem_espaco(GraphRep rep)
{
    const Aresta3 a[] = {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 4, a);
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    size_t c[2] = {77, 77};
    CHECK(dijkstra_caminho(&r, 3, c, 2) == 4);
    CHECK(c[0] == 77 && c[1] == 77);
    dijkstra_liberar(&r);
    graph_destroy(g);
}

/* Erros: origem fora do grafo e aresta de peso negativo alcançada. Nos dois
 * casos o resultado fica zerado e nada fica alocado. */
static void erros(GraphRep rep)
{
    const Aresta3 a[] = {{0, 1, 1}, {1, 2, -0.5f}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 3, a);
    size_t antes = mt_current_bytes();
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 3, &r) == DIJKSTRA_ERRO_ORIGEM);
    CHECK(r.dist == NULL && r.pred == NULL && r.n == 0);
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_ERRO_PESO_NEGATIVO);
    CHECK(r.dist == NULL && r.pred == NULL && r.n == 0);
    CHECK(mt_current_bytes() == antes);
    dijkstra_liberar(&r);   /* aceita o resultado zerado */
    graph_destroy(g);
}

/* Grafo de um vértice só, sem arestas. */
static void um_vertice(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create_weighted(1, rep, GRAPH_DIRECTED, &g) == GRAPH_OK);
    DijkstraResultado r;
    CHECK(dijkstra_executar(g, 0, &r) == DIJKSTRA_OK);
    CHECK(r.dist[0] == 0.0 && r.alcancados == 1);
    dijkstra_liberar(&r);
    graph_destroy(g);
}

/* ---- Conferência com uma implementação ingênua ---- */

/* Dijkstra sem heap, O(V²): a cada passo procura, numa varredura linear, o
 * vértice aberto de menor distância. Independente da fila de prioridade. */
static double *dijkstra_ingenuo(const Graph *g, size_t origem)
{
    size_t n = graph_num_vertices(g);
    double *d = malloc(n * sizeof *d);
    unsigned char *fechado = calloc(n, 1);
    for (size_t v = 0; v < n; v++)
        d[v] = INFINITY;
    d[origem] = 0.0;
    for (;;) {
        size_t u = SIZE_MAX;
        for (size_t v = 0; v < n; v++)
            if (!fechado[v] && !isinf(d[v]) && (u == SIZE_MAX || d[v] < d[u]))
                u = v;
        if (u == SIZE_MAX)
            break;
        fechado[u] = 1;
        GraphIter it;
        size_t w;
        float p;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next_weighted(&it, &w, &p))
            if (d[u] + p < d[w])
                d[w] = d[u] + p;
    }
    free(fechado);
    return d;
}

static unsigned long proximo(unsigned long *s)
{
    *s = (*s * 1103515245UL + 12345UL) & 0x7fffffffUL;
    return *s;
}

/* Grafo aleatório denso em atualizações de distância: exercita a subida no
 * heap (diminuir a prioridade) muitas vezes. Pesos múltiplos de 1/64, para
 * as somas serem exatas e a comparação poder ser por igualdade. */
static void aleatorio_confere_com_ingenuo(GraphRep rep)
{
    const size_t n = 300;
    Graph *g;
    graph_create_weighted(n, rep, GRAPH_DIRECTED, &g);
    unsigned long s = 66;
    for (int i = 0; i < 6000; i++) {
        size_t u = proximo(&s) % n, v = proximo(&s) % n;
        graph_add_weighted_edge(g, u, v, (float)(proximo(&s) % 640) / 64.0f);
    }
    for (size_t origem = 0; origem < n; origem += 37) {
        DijkstraResultado r;
        CHECK(dijkstra_executar(g, origem, &r) == DIJKSTRA_OK);
        double *d = dijkstra_ingenuo(g, origem);
        int iguais = 1, coerentes = 1;
        size_t alcancados = 0;
        for (size_t v = 0; v < n; v++) {
            if (!(r.dist[v] == d[v] || (isinf(r.dist[v]) && isinf(d[v]))))
                iguais = 0;
            if (!caminho_coerente(g, &r, v))
                coerentes = 0;
            alcancados += !isinf(d[v]);
        }
        CHECK(iguais);
        CHECK(coerentes);
        CHECK(r.alcancados == alcancados);
        free(d);
        dijkstra_liberar(&r);
    }
    graph_destroy(g);
}

/* ---- Dados reais: amostras e recorte ---- */

typedef struct {
    EdgeList el;
    IdMap ids;
} Arquivo;

static void ler(const char *caminho, Arquivo *a)
{
    if (edgelist_ler(caminho, &a->el) != EDGELIST_OK) {
        fprintf(stderr, "não foi possível ler %s (rode a partir da raiz do repositório)\n",
                caminho);
        exit(2);
    }
    idmap_iniciar(&a->ids);
}

static void descartar(Arquivo *a)
{
    idmap_liberar(&a->ids);
    edgelist_liberar(&a->el);
}

static size_t indice(const IdMap *ids, unsigned long long id)
{
    size_t i;
    if (!idmap_buscar(ids, id, &i)) {
        fprintf(stderr, "ID %llu ausente\n", id);
        exit(2);
    }
    return i;
}

/* Peso sintético de {a, b}, a partir dos IDs originais, enquanto os custos
 * da #65 não existem: simétrico, entre 1/1024 e 1009/1024, múltiplo de
 * 1/1024 (somas exatas em double). Fácil de reproduzir fora do C. */
static float peso_sintetico(unsigned long long a, unsigned long long b)
{
    return (float)((a ^ b) % 1009 + 1) / 1024.0f;
}

/* Copia o grafo sem peso para um com o peso sintético. */
static Graph *com_peso_sintetico(const Graph *g, const IdMap *ids)
{
    size_t n = graph_num_vertices(g);
    Graph *h;
    if (graph_create_weighted(n, graph_rep(g), graph_view(g), &h) != GRAPH_OK)
        exit(2);
    for (size_t u = 0; u < n; u++) {
        GraphIter it;
        size_t w;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next(&it, &w))
            graph_add_weighted_edge(h, u, w,
                                    peso_sintetico(idmap_original(ids, u), idmap_original(ids, w)));
    }
    return h;
}

/* Com todo peso igual a 1 (grafo sem peso), as distâncias são as do BFS,
 * vértice a vértice, nas amostras de 1000 vértices. */
static void coincide_com_bfs(GraphRep rep, GraphView view, const char *amostra,
                             unsigned long long semente)
{
    Arquivo a;
    ler(amostra, &a);
    Graph *g;
    graph_build(&a.el, &a.ids, rep, view, &g);
    size_t s = indice(&a.ids, semente);

    DijkstraResultado r;
    BfsResultado b;
    CHECK(dijkstra_executar(g, s, &r) == DIJKSTRA_OK);
    CHECK(bfs_executar(g, s, &b) == BFS_OK);
    int iguais = r.alcancados == b.alcancados;
    for (size_t v = 0; v < r.n && iguais; v++)
        iguais = b.visitado[v] ? r.dist[v] == (double)b.dist[v] : isinf(r.dist[v]);
    CHECK(iguais);
    CHECK(r.alcancados > 1);
    bfs_liberar(&b);
    dijkstra_liberar(&r);
    graph_destroy(g);
    descartar(&a);
}

/* A mesma amostra com peso, em lista e em matriz: distâncias e
 * predecessores idênticos. */
static void lista_e_matriz_identicas(const char *amostra, unsigned long long semente)
{
    Arquivo a;
    ler(amostra, &a);
    Graph *gl, *gm;
    graph_build(&a.el, &a.ids, GRAPH_LIST, GRAPH_DIRECTED, &gl);
    graph_build(&a.el, &a.ids, GRAPH_MATRIX, GRAPH_DIRECTED, &gm);
    Graph *l = com_peso_sintetico(gl, &a.ids), *m = com_peso_sintetico(gm, &a.ids);
    size_t s = indice(&a.ids, semente);

    DijkstraResultado rl, rm;
    CHECK(dijkstra_executar(l, s, &rl) == DIJKSTRA_OK);
    CHECK(dijkstra_executar(m, s, &rm) == DIJKSTRA_OK);
    CHECK(rl.alcancados == rm.alcancados);
    CHECK(memcmp(rl.dist, rm.dist, rl.n * sizeof *rl.dist) == 0);
    CHECK(memcmp(rl.pred, rm.pred, rl.n * sizeof *rl.pred) == 0);
    dijkstra_liberar(&rl);
    dijkstra_liberar(&rm);
    graph_destroy(l);
    graph_destroy(m);
    graph_destroy(gl);
    graph_destroy(gm);
    descartar(&a);
}

/* No recorte de 10 mil vértices, com o peso sintético, as distâncias batem
 * com as do Dijkstra ingênuo. Só com lista: a matriz com peso do recorte
 * ocupa 400 MB, demais para um teste de unidade. */
static void recorte_confere_com_ingenuo(void)
{
    Arquivo a;
    ler("data/recorte_n10000.txt", &a);
    Graph *g0;
    graph_build(&a.el, &a.ids, GRAPH_LIST, GRAPH_DIRECTED, &g0);
    Graph *g = com_peso_sintetico(g0, &a.ids);
    size_t s = indice(&a.ids, 307642294ULL);

    DijkstraResultado r;
    CHECK(dijkstra_executar(g, s, &r) == DIJKSTRA_OK);
    double *d = dijkstra_ingenuo(g, s);
    int iguais = 1;
    for (size_t v = 0; v < r.n; v++)
        if (!(r.dist[v] == d[v] || (isinf(r.dist[v]) && isinf(d[v]))))
            iguais = 0;
    CHECK(iguais);
    CHECK(r.alcancados > 1000);
    free(d);
    dijkstra_liberar(&r);
    graph_destroy(g);
    graph_destroy(g0);
    descartar(&a);
}

static void rodar(GraphRep rep)
{
    caminho_calculado_a_mao(rep);
    pesos_fracionarios(rep);
    empate(rep);
    inalcancavel(rep);
    sem_sentido(rep);
    caminho_sem_espaco(rep);
    erros(rep);
    um_vertice(rep);
    aleatorio_confere_com_ingenuo(rep);
}

int main(void)
{
    const char *amostra = "data/samples/bfs_307642294_n1000.txt";
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);
    for (GraphRep rep = GRAPH_LIST; rep <= GRAPH_MATRIX; rep++) {
        coincide_com_bfs(rep, GRAPH_DIRECTED, amostra, 307642294ULL);
        coincide_com_bfs(rep, GRAPH_SYMMETRIC, amostra, 307642294ULL);
        /* 9487272 não segue ninguém na própria amostra: a direcionada nem sai dela */
        coincide_com_bfs(rep, GRAPH_DIRECTED, "data/samples/bfs_144319796_n1000.txt",
                         144319796ULL);
        coincide_com_bfs(rep, GRAPH_SYMMETRIC, "data/samples/bfs_9487272_n1000.txt", 9487272ULL);
    }
    lista_e_matriz_identicas(amostra, 307642294ULL);
    recorte_confere_com_ingenuo();
    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
