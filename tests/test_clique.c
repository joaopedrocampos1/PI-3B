/*
 * Testes da Clique Máxima exata (src/clique.c), nas duas representações.
 *
 * Além das cliques conhecidas, compara o resultado com um Bron–Kerbosch
 * simples escrito aqui, sobre uma matriz de adjacência própria, sem passar
 * pelo TAD: em grafos aleatórios e nas amostras de 100 vértices.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_clique && ./bin/test_clique
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

static Graph *criar(GraphRep rep, size_t n)
{
    Graph *g;
    if (graph_create(n, rep, GRAPH_RECIPROCAL, &g) != GRAPH_OK) {
        fprintf(stderr, "sem memória para o grafo de teste\n");
        exit(2);
    }
    return g;
}

static Graph *montar(GraphRep rep, size_t n, const Par *a, size_t m)
{
    Graph *g = criar(rep, n);
    for (size_t i = 0; i < m; i++)
        graph_add_edge(g, a[i].u, a[i].v);
    return g;
}

#define MONTAR(rep, n, arestas) montar(rep, n, arestas, sizeof arestas / sizeof *arestas)

/* A clique devolvida é de fato uma clique: índices válidos, em ordem
 * estritamente crescente e todos vizinhos entre si. */
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

/* ---------- conferência independente: Bron–Kerbosch com pivô ---------- */

typedef struct {
    size_t n;
    unsigned char *adj;   /* n x n, 1 se {u, v} é aresta */
    size_t melhor;
} Bk;

#define ADJ(b, u, v) ((b)->adj[(u) * (b)->n + (v)])

/* R tem tam_r vértices; P e X são vetores de 0/1 com n posições. */
static void bk(Bk *b, size_t tam_r, unsigned char *P, unsigned char *X)
{
    size_t n = b->n, pivo = n, maior = 0;
    int vazio = 1;
    for (size_t u = 0; u < n; u++) {
        if (!P[u] && !X[u])
            continue;
        vazio = 0;
        size_t c = 0;
        for (size_t v = 0; v < n; v++)
            c += P[v] && ADJ(b, u, v);
        if (pivo == n || c > maior) {
            pivo = u;
            maior = c;
        }
    }
    if (vazio) {
        if (tam_r > b->melhor)
            b->melhor = tam_r;
        return;
    }
    unsigned char *P2 = malloc(n), *X2 = malloc(n);
    if (!P2 || !X2)
        exit(2);
    for (size_t v = 0; v < n; v++) {
        if (!P[v] || ADJ(b, pivo, v))
            continue;
        for (size_t w = 0; w < n; w++) {
            P2[w] = P[w] && ADJ(b, v, w);
            X2[w] = X[w] && ADJ(b, v, w);
        }
        bk(b, tam_r + 1, P2, X2);
        P[v] = 0;
        X[v] = 1;
    }
    free(P2);
    free(X2);
}

static size_t bk_maxima(Bk *b)
{
    unsigned char *P = malloc(b->n ? b->n : 1), *X = calloc(b->n ? b->n : 1, 1);
    if (!P || !X)
        exit(2);
    memset(P, 1, b->n);
    b->melhor = 0;
    bk(b, 0, P, X);
    free(P);
    free(X);
    return b->melhor;
}

/* ---------------------------- casos fixos ---------------------------- */

static void completo(GraphRep rep)
{
    Graph *g = criar(rep, 5);
    for (size_t u = 0; u < 5; u++)
        for (size_t v = u + 1; v < 5; v++)
            graph_add_edge(g, u, v);
    CliqueResultado r;
    CHECK(clique_exata(g, 0, &r) == CLIQUE_OK);
    size_t esperada[] = {0, 1, 2, 3, 4};
    CHECK(clique_e(&r, esperada, 5));
    CHECK(r.exato && r.n == 5 && r.nos >= 5);
    clique_liberar(&r);
    graph_destroy(g);
}

/* C3 é um triângulo (clique 3); de C4 em diante, a clique máxima é uma
 * aresta. */
static void ciclos(GraphRep rep)
{
    for (size_t n = 3; n <= 8; n++) {
        Graph *g = criar(rep, n);
        for (size_t v = 0; v < n; v++)
            graph_add_edge(g, v, (v + 1) % n);
        CliqueResultado r;
        CHECK(clique_exata(g, 0, &r) == CLIQUE_OK);
        CHECK(r.tam == (n == 3 ? 3 : 2));
        CHECK(clique_valida(g, &r) && r.exato);
        clique_liberar(&r);
        graph_destroy(g);
    }
}

/* Sem arestas, qualquer vértice sozinho é uma clique máxima. */
static void sem_arestas(GraphRep rep)
{
    Graph *g = criar(rep, 4);
    CliqueResultado r;
    CHECK(clique_exata(g, 0, &r) == CLIQUE_OK);
    CHECK(r.tam == 1 && clique_valida(g, &r) && r.exato);
    clique_liberar(&r);
    graph_destroy(g);
}

static void vazio(GraphRep rep)
{
    Graph *g = criar(rep, 0);
    CliqueResultado r;
    CHECK(clique_exata(g, 0, &r) == CLIQUE_OK);
    CHECK(r.n == 0 && r.tam == 0 && r.clique == NULL && r.exato);
    clique_liberar(&r);
    CHECK(r.clique == NULL && r.tam == 0);
    clique_liberar(&r);   /* liberar de novo um resultado zerado */
    graph_destroy(g);
}

/* Um K4 {0, 1, 2, 3} e um K5 {3, 4, 5, 6, 7} dividindo o vértice 3. */
static void k4_e_k5_dividindo_vertice(GraphRep rep)
{
    Graph *g = criar(rep, 8);
    size_t a[] = {0, 1, 2, 3}, b[] = {3, 4, 5, 6, 7};
    for (size_t i = 0; i < 4; i++)
        for (size_t j = i + 1; j < 4; j++)
            graph_add_edge(g, a[i], a[j]);
    for (size_t i = 0; i < 5; i++)
        for (size_t j = i + 1; j < 5; j++)
            graph_add_edge(g, b[i], b[j]);
    CliqueResultado r;
    CHECK(clique_exata(g, 0, &r) == CLIQUE_OK);
    CHECK(clique_e(&r, b, 5) && r.exato);
    clique_liberar(&r);
    graph_destroy(g);
}

/* Dois K4, {0, 1, 2, 3} e {2, 3, 4, 5}, dividindo a aresta {2, 3}: a clique
 * máxima tem 4 vértices e é um dos dois, nunca uma mistura. */
static void dois_k4_dividindo_aresta(GraphRep rep)
{
    Graph *g = criar(rep, 6);
    size_t a[] = {0, 1, 2, 3}, b[] = {2, 3, 4, 5};
    for (size_t i = 0; i < 4; i++)
        for (size_t j = i + 1; j < 4; j++) {
            graph_add_edge(g, a[i], a[j]);
            graph_add_edge(g, b[i], b[j]);
        }
    CliqueResultado r;
    CHECK(clique_exata(g, 0, &r) == CLIQUE_OK);
    CHECK(clique_e(&r, a, 4) || clique_e(&r, b, 4));
    CHECK(r.exato);
    clique_liberar(&r);
    graph_destroy(g);
}

/* O vértice de maior grau (0, o centro de uma estrela) não está na clique
 * máxima, um triângulo longe dele. */
static void maior_grau_fora_da_clique(GraphRep rep)
{
    Par a[] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {6, 7}, {7, 8}, {8, 6}};
    Graph *g = MONTAR(rep, 9, a);
    CliqueResultado r;
    CHECK(clique_exata(g, 0, &r) == CLIQUE_OK);
    size_t esperada[] = {6, 7, 8};
    CHECK(clique_e(&r, esperada, 3) && r.exato);
    clique_liberar(&r);
    graph_destroy(g);
}

/* ------------------- aleatórios, lista == matriz == BK ------------------- */

static unsigned long long rng = 88172645463325252ULL;

static unsigned long long sortear(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static void aleatorios(void)
{
    static const size_t tamanhos[] = {10, 30, 60, 90};
    static const unsigned densidades[] = {10, 30, 50, 70};   /* em % */
    for (size_t a = 0; a < sizeof tamanhos / sizeof *tamanhos; a++)
        for (size_t d = 0; d < sizeof densidades / sizeof *densidades; d++) {
            size_t n = tamanhos[a];
            Graph *gl = criar(GRAPH_LIST, n), *gm = criar(GRAPH_MATRIX, n);
            Bk b = {n, calloc(n * n, 1), 0};
            if (!b.adj)
                exit(2);
            for (size_t u = 0; u < n; u++)
                for (size_t v = u + 1; v < n; v++)
                    if (sortear() % 100 < densidades[d]) {
                        graph_add_edge(gl, u, v);
                        graph_add_edge(gm, u, v);
                        ADJ(&b, u, v) = ADJ(&b, v, u) = 1;
                    }
            CliqueResultado rl, rm;
            CHECK(clique_exata(gl, 0, &rl) == CLIQUE_OK);
            CHECK(clique_exata(gm, 0, &rm) == CLIQUE_OK);
            CHECK(clique_valida(gl, &rl) && rl.exato);
            CHECK(rl.tam == bk_maxima(&b));
            /* resultado idêntico, até no número de nós da busca */
            CHECK(clique_e(&rm, rl.clique, rl.tam) && rm.nos == rl.nos);
            clique_liberar(&rl);
            clique_liberar(&rm);
            free(b.adj);
            graph_destroy(gl);
            graph_destroy(gm);
        }
}

/* Com um limite de tempo minúsculo num grafo denso, a busca para antes de
 * terminar, avisa que não é exato e mesmo assim devolve uma clique. */
static void limite_de_tempo(GraphRep rep)
{
    size_t n = 300;
    Graph *g = criar(rep, n);
    for (size_t u = 0; u < n; u++)
        for (size_t v = u + 1; v < n; v++)
            if (sortear() % 100 < 90)
                graph_add_edge(g, u, v);
    CliqueResultado r;
    CHECK(clique_exata(g, 1e-6, &r) == CLIQUE_OK);
    CHECK(!r.exato);
    CHECK(r.tam >= 1 && clique_valida(g, &r));
    clique_liberar(&r);
    graph_destroy(g);
}

/* ------------------- amostras de 100 vértices ------------------- */

/* Visão recíproca montada direto da EdgeList: {a, b} é aresta quando a
 * segue b e b segue a. Os índices vêm de um IdMap próprio. */
static void conferir_amostra(const char *caminho)
{
    EdgeList el;
    CHECK(edgelist_ler(caminho, &el) == EDGELIST_OK);
    if (el.n == 0) {
        fprintf(stderr, "  amostra vazia ou ausente: %s\n", caminho);
        return;
    }

    IdMap meus;
    idmap_iniciar(&meus);
    for (size_t i = 0; i < el.n; i++) {
        size_t x;
        CHECK(idmap_inserir(&meus, el.arestas[i].origem, &x));
        CHECK(idmap_inserir(&meus, el.arestas[i].destino, &x));
    }
    size_t n = meus.n;
    unsigned char *dir = calloc(n * n, 1);
    Bk b = {n, calloc(n * n, 1), 0};
    if (!dir || !b.adj)
        exit(2);
    for (size_t i = 0; i < el.n; i++) {
        size_t u, v;
        idmap_buscar(&meus, el.arestas[i].origem, &u);
        idmap_buscar(&meus, el.arestas[i].destino, &v);
        dir[u * n + v] = 1;
    }
    for (size_t u = 0; u < n; u++)
        for (size_t v = 0; v < n; v++)
            ADJ(&b, u, v) = dir[u * n + v] && dir[v * n + u];
    size_t esperado = bk_maxima(&b);

    CliqueResultado res[2];
    GraphRep reps[2] = {GRAPH_LIST, GRAPH_MATRIX};
    for (int k = 0; k < 2; k++) {
        IdMap ids;
        idmap_iniciar(&ids);
        Graph *g;
        CHECK(graph_build(&el, &ids, reps[k], GRAPH_RECIPROCAL, &g) == GRAPH_OK);
        CHECK(clique_exata(g, 0, &res[k]) == CLIQUE_OK);
        CHECK(res[k].exato && res[k].tam == esperado && clique_valida(g, &res[k]));

        /* a clique também é clique na matriz independente */
        int confere = 1;
        for (size_t i = 0; i < res[k].tam; i++)
            for (size_t j = 0; j < i; j++) {
                size_t u, v;
                confere &= idmap_buscar(&meus, idmap_original(&ids, res[k].clique[i]), &u);
                confere &= idmap_buscar(&meus, idmap_original(&ids, res[k].clique[j]), &v);
                confere &= ADJ(&b, u, v);
            }
        CHECK(confere);
        graph_destroy(g);
        idmap_liberar(&ids);
    }
    CHECK(clique_e(&res[1], res[0].clique, res[0].tam) && res[0].nos == res[1].nos);
    printf("  %s: clique de %zu, %llu nós\n", caminho, res[0].tam, res[0].nos);

    clique_liberar(&res[0]);
    clique_liberar(&res[1]);
    free(dir);
    free(b.adj);
    idmap_liberar(&meus);
    edgelist_liberar(&el);
}

static void rodar(GraphRep rep)
{
    completo(rep);
    ciclos(rep);
    sem_arestas(rep);
    vazio(rep);
    k4_e_k5_dividindo_vertice(rep);
    dois_k4_dividindo_aresta(rep);
    maior_grau_fora_da_clique(rep);
    limite_de_tempo(rep);
}

int main(void)
{
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);
    aleatorios();

    static const char *amostras[] = {
        "data/samples/bfs_144319796_n100.txt",
        "data/samples/bfs_307642294_n100.txt",
        "data/samples/bfs_9487272_n100.txt",
    };
    for (size_t i = 0; i < sizeof amostras / sizeof *amostras; i++)
        conferir_amostra(amostras[i]);

    CHECK(mt_current_bytes() == 0);   /* nada ficou alocado pelo memtrack */

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
