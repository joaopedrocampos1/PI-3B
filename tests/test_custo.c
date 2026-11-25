/*
 * Testes do custo por vizinhos em comum (src/custo.c), nas duas
 * representações.
 *
 * Além de um grafo com custos calculados à mão, confere os custos do recorte
 * de 10 mil vértices (#59) com uma implementação independente, escrita aqui
 * sobre listas de adjacência próprias, sem passar pelo TAD.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_custo && ./bin/test_custo
 */
#include "custo.h"
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

static Graph *criar(GraphRep rep, GraphView view, size_t n)
{
    Graph *g;
    if (graph_create(n, rep, view, &g) != GRAPH_OK) {
        fprintf(stderr, "sem memória para o grafo de teste\n");
        exit(2);
    }
    return g;
}

/* As duas visões do mesmo conjunto de arestas "u segue v". */
static void montar(GraphRep rep, size_t n, const Par *a, size_t m, Graph **dir, Graph **sim)
{
    *dir = criar(rep, GRAPH_DIRECTED, n);
    *sim = criar(rep, GRAPH_SYMMETRIC, n);
    for (size_t i = 0; i < m; i++) {
        graph_add_edge(*dir, a[i].u, a[i].v);
        graph_add_edge(*sim, a[i].u, a[i].v);
    }
}

/*
 * Grafo pequeno, com os custos à mão. Arestas "u segue v":
 *   0 -> 1, 1 -> 0, 0 -> 2, 1 -> 2, 2 -> 3, 3 -> 4, 4 -> 3
 * Círculos (visão simetrizada):
 *   N(0) = {1, 2}   N(1) = {0, 2}   N(2) = {0, 1, 3}   N(3) = {2, 4}   N(4) = {3}
 *
 *   aresta   N(A) ∩ N(B)   mútua   c   custo
 *   0 -> 1   {2}           sim     2   1/3
 *   1 -> 0   {2}           sim     2   1/3
 *   0 -> 2   {1}           não     1   1/2
 *   1 -> 2   {0}           não     1   1/2
 *   2 -> 3   {}            não     0   1
 *   3 -> 4   {}            sim     1   1/2
 *   4 -> 3   {}            sim     1   1/2
 */
static const Par MAO[] = {{0, 1}, {1, 0}, {0, 2}, {1, 2}, {2, 3}, {3, 4}, {4, 3}};
#define N_MAO 5
#define M_MAO (sizeof MAO / sizeof *MAO)

static void calculado_a_mao(GraphRep rep)
{
    Graph *dir, *sim;
    montar(rep, N_MAO, MAO, M_MAO, &dir, &sim);

    CHECK(custo_comuns(sim, 0, 1) == 1 && custo_comuns(sim, 1, 0) == 1);
    CHECK(custo_comuns(sim, 2, 3) == 0 && custo_comuns(sim, 0, 3) == 1);
    CHECK(custo_aresta(dir, sim, 0, 1) == 1.0f / 3.0f);
    CHECK(custo_aresta(dir, sim, 0, 2) == 0.5f);
    CHECK(custo_aresta(dir, sim, 2, 3) == 1.0f);
    CHECK(custo_aresta(dir, sim, 3, 4) == 0.5f);

    /* propagação: "A segue B" vira B -> A */
    Graph *g;
    CustoResumo r;
    CHECK(custo_construir(dir, sim, rep, CUSTO_SENTIDO_PROPAGACAO, &g, &r) == CUSTO_OK);
    CHECK(graph_is_weighted(g) && graph_view(g) == GRAPH_DIRECTED);
    CHECK(graph_num_vertices(g) == N_MAO && graph_num_edges(g) == M_MAO);
    CHECK(graph_edge_weight(g, 1, 0) == 1.0f / 3.0f);
    CHECK(graph_edge_weight(g, 0, 1) == 1.0f / 3.0f);
    CHECK(graph_edge_weight(g, 2, 0) == 0.5f && !graph_has_edge(g, 0, 2));
    CHECK(graph_edge_weight(g, 2, 1) == 0.5f && !graph_has_edge(g, 1, 2));
    CHECK(graph_edge_weight(g, 3, 2) == 1.0f && !graph_has_edge(g, 2, 3));
    CHECK(graph_edge_weight(g, 4, 3) == 0.5f && graph_edge_weight(g, 3, 4) == 0.5f);

    CHECK(r.arestas == M_MAO && r.mutuas == 4);
    CHECK(r.tam_hist == 3);
    CHECK(r.hist && r.hist[0] == 1 && r.hist[1] == 4 && r.hist[2] == 2);
    custo_resumo_liberar(&r);
    CHECK(r.hist == NULL && r.tam_hist == 0);
    custo_resumo_liberar(&r);   /* liberar de novo um resumo zerado */
    graph_destroy(g);

    /* sentido "segue": as mesmas arestas da visão direcionada */
    CHECK(custo_construir(dir, sim, rep, CUSTO_SENTIDO_SEGUE, &g, NULL) == CUSTO_OK);
    for (size_t i = 0; i < M_MAO; i++)
        CHECK(graph_edge_weight(g, MAO[i].u, MAO[i].v) ==
              custo_aresta(dir, sim, MAO[i].u, MAO[i].v));
    CHECK(graph_has_edge(g, 2, 3) && !graph_has_edge(g, 3, 2));
    graph_destroy(g);

    graph_destroy(dir);
    graph_destroy(sim);
}

/* Visões trocadas ou de tamanhos diferentes são recusadas. */
static void visoes_erradas(GraphRep rep)
{
    Graph *dir, *sim, *g = (Graph *)&g;
    montar(rep, N_MAO, MAO, M_MAO, &dir, &sim);
    CustoResumo r;
    CHECK(custo_construir(sim, dir, rep, CUSTO_SENTIDO_PROPAGACAO, &g, &r) == CUSTO_ERRO_VISAO);
    CHECK(g == NULL && r.hist == NULL && r.arestas == 0);
    CHECK(custo_construir(dir, dir, rep, CUSTO_SENTIDO_PROPAGACAO, &g, NULL) == CUSTO_ERRO_VISAO);
    Graph *outro = criar(rep, GRAPH_SYMMETRIC, N_MAO + 1);
    CHECK(custo_construir(dir, outro, rep, CUSTO_SENTIDO_PROPAGACAO, &g, NULL) == CUSTO_ERRO_VISAO);
    graph_destroy(outro);
    graph_destroy(dir);
    graph_destroy(sim);
}

/* Sem vértices ou sem arestas: grafo com peso vazio, resumo vazio. */
static void vazios(GraphRep rep)
{
    for (size_t n = 0; n <= 3; n += 3) {
        Graph *dir, *sim, *g;
        montar(rep, n, NULL, 0, &dir, &sim);
        CustoResumo r;
        CHECK(custo_construir(dir, sim, rep, CUSTO_SENTIDO_PROPAGACAO, &g, &r) == CUSTO_OK);
        CHECK(g && graph_num_vertices(g) == n && graph_num_edges(g) == 0);
        CHECK(r.arestas == 0 && r.tam_hist == 0 && r.hist == NULL);
        custo_resumo_liberar(&r);
        graph_destroy(g);
        graph_destroy(dir);
        graph_destroy(sim);
    }
}

/* -------------- aleatórios: lista == matriz, peso a peso -------------- */

static unsigned long long rng = 88172645463325252ULL;

static unsigned long long sortear(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static void lista_igual_matriz(void)
{
    static const unsigned densidades[] = {5, 20, 50};   /* em % */
    for (size_t d = 0; d < sizeof densidades / sizeof *densidades; d++) {
        size_t n = 80;
        Graph *dir[2], *sim[2], *g[2];
        GraphRep reps[2] = {GRAPH_LIST, GRAPH_MATRIX};
        for (int k = 0; k < 2; k++) {
            dir[k] = criar(reps[k], GRAPH_DIRECTED, n);
            sim[k] = criar(reps[k], GRAPH_SYMMETRIC, n);
        }
        for (size_t u = 0; u < n; u++)
            for (size_t v = 0; v < n; v++)
                if (u != v && sortear() % 100 < densidades[d])
                    for (int k = 0; k < 2; k++) {
                        graph_add_edge(dir[k], u, v);
                        graph_add_edge(sim[k], u, v);
                    }
        CustoResumo r[2];
        for (int k = 0; k < 2; k++)
            CHECK(custo_construir(dir[k], sim[k], reps[k], CUSTO_SENTIDO_PROPAGACAO, &g[k],
                                  &r[k]) == CUSTO_OK);

        int iguais = graph_num_edges(g[0]) == graph_num_edges(g[1]);
        for (size_t u = 0; u < n; u++)
            for (size_t v = 0; v < n; v++) {
                float a = graph_edge_weight(g[0], u, v), b = graph_edge_weight(g[1], u, v);
                iguais &= a == b;   /* inclusive INFINITY == INFINITY */
            }
        CHECK(iguais);
        CHECK(r[0].arestas == r[1].arestas && r[0].mutuas == r[1].mutuas &&
              r[0].tam_hist == r[1].tam_hist &&
              memcmp(r[0].hist, r[1].hist, r[0].tam_hist * sizeof *r[0].hist) == 0);

        for (int k = 0; k < 2; k++) {
            custo_resumo_liberar(&r[k]);
            graph_destroy(g[k]);
            graph_destroy(dir[k]);
            graph_destroy(sim[k]);
        }
    }
}

/* ---------- recorte de 10 mil vértices: implementação independente ---------- */

typedef struct {
    size_t n;
    size_t *ini;     /* círculo de v: viz[ini[v] .. ini[v+1]-1], em ordem crescente */
    size_t *viz;
} Circulos;

static int cmp_par(const void *a, const void *b)
{
    const Par *x = a, *y = b;
    if (x->u != y->u)
        return (x->u > y->u) - (x->u < y->u);
    return (x->v > y->v) - (x->v < y->v);
}

/* Interseção de dois vetores ordenados, por intercalação. */
static size_t intersecao(const Circulos *c, size_t a, size_t b)
{
    size_t i = c->ini[a], fi = c->ini[a + 1], j = c->ini[b], fj = c->ini[b + 1], k = 0;
    while (i < fi && j < fj) {
        if (c->viz[i] < c->viz[j])
            i++;
        else if (c->viz[i] > c->viz[j])
            j++;
        else {
            k++;
            i++;
            j++;
        }
    }
    return k;
}

static void recorte(void)
{
    const char *caminho = "data/recorte_n10000.txt";
    EdgeList el;
    CHECK(edgelist_ler(caminho, &el) == EDGELIST_OK);
    if (el.n == 0) {
        fprintf(stderr, "  recorte vazio ou ausente: %s\n", caminho);
        return;
    }

    /* índices próprios e arestas "u segue v" ordenadas, para a busca da volta */
    IdMap meus;
    idmap_iniciar(&meus);
    Par *seg = malloc(el.n * sizeof *seg);
    Par *pares = malloc(2 * el.n * sizeof *pares);
    if (!seg || !pares)
        exit(2);
    for (size_t i = 0; i < el.n; i++) {
        CHECK(idmap_inserir(&meus, el.arestas[i].origem, &seg[i].u));
        CHECK(idmap_inserir(&meus, el.arestas[i].destino, &seg[i].v));
        pares[2 * i] = seg[i];
        pares[2 * i + 1] = (Par){seg[i].v, seg[i].u};
    }
    qsort(seg, el.n, sizeof *seg, cmp_par);

    /* círculos: pares nos dois sentidos, ordenados e sem repetição */
    size_t n = meus.n, m2 = 2 * el.n;
    qsort(pares, m2, sizeof *pares, cmp_par);
    Circulos c = {n, calloc(n + 1, sizeof(size_t)), malloc(m2 * sizeof(size_t))};
    if (!c.ini || !c.viz)
        exit(2);
    size_t k = 0;
    for (size_t i = 0; i < m2; i++) {
        if (i > 0 && pares[i].u == pares[i - 1].u && pares[i].v == pares[i - 1].v)
            continue;
        c.viz[k++] = pares[i].v;
        c.ini[pares[i].u + 1]++;
    }
    for (size_t v = 0; v < n; v++)
        c.ini[v + 1] += c.ini[v];

    /* o TAD, com o custo no sentido da propagação, em lista */
    IdMap ids;
    idmap_iniciar(&ids);
    Graph *dir, *sim, *g;
    CHECK(graph_build(&el, &ids, GRAPH_LIST, GRAPH_DIRECTED, &dir) == GRAPH_OK);
    CHECK(graph_build(&el, &ids, GRAPH_LIST, GRAPH_SYMMETRIC, &sim) == GRAPH_OK);
    CustoResumo r;
    CHECK(custo_construir(dir, sim, GRAPH_LIST, CUSTO_SENTIDO_PROPAGACAO, &g, &r) == CUSTO_OK);
    CHECK(graph_num_edges(g) == el.n && r.arestas == el.n);

    size_t *hist = calloc(n + 1, sizeof *hist), maior = 0, mutuas = 0;
    if (!hist)
        exit(2);
    int confere = 1;
    for (size_t i = 0; i < el.n; i++) {
        size_t a = seg[i].u, b = seg[i].v;
        Par volta = {b, a};
        int mutua = bsearch(&volta, seg, el.n, sizeof *seg, cmp_par) != NULL;
        size_t cc = intersecao(&c, a, b) + (size_t)mutua;
        hist[cc]++;
        mutuas += (size_t)mutua;
        if (cc > maior)
            maior = cc;

        /* a mesma aresta no grafo do TAD, pelos IDs originais */
        size_t ta, tb;
        confere &= idmap_buscar(&ids, idmap_original(&meus, a), &ta);
        confere &= idmap_buscar(&ids, idmap_original(&meus, b), &tb);
        confere &= graph_edge_weight(g, tb, ta) == 1.0f / (1.0f + (float)cc);
    }
    CHECK(confere);
    CHECK(r.mutuas == mutuas && r.tam_hist == maior + 1);
    CHECK(r.hist && memcmp(r.hist, hist, r.tam_hist * sizeof *hist) == 0);

    printf("  %s: %zu arestas, %zu mútuas, custo 1 em %zu (%.1f%%), maior c = %zu\n", caminho,
           r.arestas, r.mutuas, r.hist[0], 100.0 * (double)r.hist[0] / (double)r.arestas,
           r.tam_hist - 1);

    custo_resumo_liberar(&r);
    graph_destroy(g);
    graph_destroy(dir);
    graph_destroy(sim);
    idmap_liberar(&ids);
    free(hist);
    free(c.ini);
    free(c.viz);
    free(pares);
    free(seg);
    idmap_liberar(&meus);
    edgelist_liberar(&el);
}

int main(void)
{
    GraphRep reps[2] = {GRAPH_LIST, GRAPH_MATRIX};
    for (int k = 0; k < 2; k++) {
        calculado_a_mao(reps[k]);
        visoes_erradas(reps[k]);
        vazios(reps[k]);
    }
    lista_igual_matriz();
    recorte();

    CHECK(mt_current_bytes() == 0);   /* nada ficou alocado pelo memtrack */

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
