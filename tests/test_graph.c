/*
 * Testes do TAD Grafo (src/graph.c) e das duas representações: lista
 * (src/graph_list.c) e matriz de adjacência (src/graph_matrix.c). Os casos
 * gerais rodam uma vez para cada representação.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_graph && ./bin/test_graph
 */
#include "graph.h"
#include "memtrack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIXTURE "bin/fixture_graph.txt"

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

/* Lê um grafo escrito como edge list, pelo mesmo caminho do programa real. */
static EdgeList arestas(const char *conteudo)
{
    FILE *f = fopen(FIXTURE, "wb");
    if (!f) {
        perror(FIXTURE);
        exit(2);
    }
    fputs(conteudo, f);
    fclose(f);

    EdgeList el;
    if (edgelist_ler(FIXTURE, &el) != EDGELIST_OK) {
        fprintf(stderr, "não foi possível ler a fixture\n");
        exit(2);
    }
    return el;
}

/* Vizinhos de v, na ordem do iterador, em `viz`. Devolve quantos são. */
static size_t vizinhos(const Graph *g, size_t v, size_t *viz, size_t max)
{
    GraphIter it;
    size_t w, k = 0;
    graph_neighbors_begin(g, v, &it);
    while (graph_neighbors_next(&it, &w))
        if (k < max)
            viz[k++] = w;
        else
            k++;
    return k;
}

static size_t indice(const IdMap *ids, unsigned long long id)
{
    size_t i;
    if (!idmap_buscar(ids, id, &i)) {
        fprintf(stderr, "ID %llu não mapeado\n", id);
        exit(2);
    }
    return i;
}

/* Inserção fora de ordem: o iterador devolve os vizinhos ordenados, e
 * repetições e laços não contam. */
static void insercao_e_vizinhos(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create(5, rep, GRAPH_DIRECTED, &g) == GRAPH_OK);
    CHECK(graph_num_vertices(g) == 5);
    CHECK(graph_num_edges(g) == 0);

    size_t ordem[] = {3, 1, 4, 2};
    for (size_t k = 0; k < 4; k++)
        CHECK(graph_add_edge(g, 0, ordem[k]) == GRAPH_OK);
    CHECK(graph_add_edge(g, 0, 3) == GRAPH_OK);   /* repetida */
    CHECK(graph_add_edge(g, 2, 2) == GRAPH_OK);   /* laço */

    CHECK(graph_num_edges(g) == 4);
    CHECK(graph_degree(g, 0) == 4);
    CHECK(graph_degree(g, 2) == 0);

    size_t viz[8];
    CHECK(vizinhos(g, 0, viz, 8) == 4);
    CHECK(viz[0] == 1 && viz[1] == 2 && viz[2] == 3 && viz[3] == 4);
    CHECK(vizinhos(g, 4, viz, 8) == 0);

    CHECK(graph_has_edge(g, 0, 4));
    CHECK(!graph_has_edge(g, 4, 0));   /* direcionado */
    CHECK(!graph_has_edge(g, 2, 2));

    graph_destroy(g);
}

/* Vizinhos crescendo além da capacidade inicial do vetor. */
static void estrela_grande(GraphRep rep)
{
    enum { N = 1000 };
    Graph *g;
    CHECK(graph_create(N, rep, GRAPH_SYMMETRIC, &g) == GRAPH_OK);
    for (size_t v = N - 1; v >= 1; v--)   /* de trás para frente: pior caso */
        CHECK(graph_add_edge(g, 0, v) == GRAPH_OK);

    CHECK(graph_num_edges(g) == N - 1);
    CHECK(graph_degree(g, 0) == N - 1);
    CHECK(graph_degree(g, 500) == 1);

    GraphIter it;
    size_t w, esperado = 1, crescente = 1;
    graph_neighbors_begin(g, 0, &it);
    while (graph_neighbors_next(&it, &w))
        crescente &= (w == esperado++);
    CHECK(crescente && esperado == N);

    graph_destroy(g);
}

/* Arestas: 1->2, 2->1, 2->3, 3->4, 4->1, 5->6.
 * Mútuo só o par {1, 2}. Índices por ordem de aparição: 1:0 2:1 3:2 4:3 5:4 6:5 */
static void tres_visoes(GraphRep rep)
{
    EdgeList el = arestas("2 1\n1 2\n2 3\n3 4\n4 1\n5 6\n");
    IdMap ids;
    idmap_iniciar(&ids);
    Graph *dir, *sim, *rec;
    CHECK(graph_build(&el, &ids, rep, GRAPH_DIRECTED, &dir) == GRAPH_OK);
    CHECK(graph_build(&el, &ids, rep, GRAPH_SYMMETRIC, &sim) == GRAPH_OK);
    CHECK(graph_build(&el, &ids, rep, GRAPH_RECIPROCAL, &rec) == GRAPH_OK);
    CHECK(ids.n == 6);   /* o mesmo IdMap nas três: nenhum índice novo */

    size_t v1 = indice(&ids, 1), v2 = indice(&ids, 2), v3 = indice(&ids, 3),
           v4 = indice(&ids, 4), v5 = indice(&ids, 5), v6 = indice(&ids, 6);

    CHECK(graph_num_vertices(dir) == 6);
    CHECK(graph_num_edges(dir) == 6);
    CHECK(graph_has_edge(dir, v2, v3) && !graph_has_edge(dir, v3, v2));
    CHECK(graph_degree(dir, v2) == 2);
    CHECK(graph_degree(dir, v6) == 0);

    /* {1,2} vira uma aresta só */
    CHECK(graph_num_edges(sim) == 5);
    CHECK(graph_has_edge(sim, v3, v2) && graph_has_edge(sim, v2, v3));
    CHECK(graph_has_edge(sim, v1, v4));
    CHECK(graph_degree(sim, v1) == 2);
    CHECK(graph_degree(sim, v6) == 1);

    CHECK(graph_num_vertices(rec) == 6);
    CHECK(graph_num_edges(rec) == 1);
    CHECK(graph_has_edge(rec, v1, v2) && graph_has_edge(rec, v2, v1));
    CHECK(!graph_has_edge(rec, v2, v3));
    CHECK(graph_degree(rec, v5) == 0);

    CHECK(graph_view(rec) == GRAPH_RECIPROCAL);
    CHECK(graph_rep(rec) == rep);

    graph_destroy(dir);
    graph_destroy(sim);
    graph_destroy(rec);
    idmap_liberar(&ids);
    edgelist_liberar(&el);
}

/* Ciclo 1->2->3->1 sem nenhum par mútuo: a visão recíproca fica sem arestas,
 * mas com os vértices. */
static void reciproca_vazia(GraphRep rep)
{
    EdgeList el = arestas("1 2\n2 3\n3 1\n");
    IdMap ids;
    idmap_iniciar(&ids);
    Graph *rec;
    CHECK(graph_build(&el, &ids, rep, GRAPH_RECIPROCAL, &rec) == GRAPH_OK);
    CHECK(graph_num_vertices(rec) == 3);
    CHECK(graph_num_edges(rec) == 0);
    graph_destroy(rec);
    idmap_liberar(&ids);
    edgelist_liberar(&el);
}

/* Matriz de bits: arestas nas bordas das palavras de 64 bits (63/64, 127/128)
 * e na última coluna, com n que não é múltiplo de 64. */
static void matriz_bordas_de_palavra(void)
{
    enum { N = 130 };
    size_t cols[] = {0, 63, 64, 127, 128, 129};
    enum { K = sizeof cols / sizeof cols[0] };

    Graph *g;
    CHECK(graph_create(N, GRAPH_MATRIX, GRAPH_DIRECTED, &g) == GRAPH_OK);
    for (size_t k = K; k-- > 0;)
        CHECK(graph_add_edge(g, 129, cols[k]) == GRAPH_OK);   /* 129 -> 129 é laço */

    CHECK(graph_num_edges(g) == K - 1);
    CHECK(graph_degree(g, 129) == K - 1);
    CHECK(!graph_has_edge(g, 129, 129));
    CHECK(graph_has_edge(g, 129, 64) && !graph_has_edge(g, 129, 65));

    size_t viz[8];
    CHECK(vizinhos(g, 129, viz, 8) == K - 1);
    int ok = 1;
    for (size_t k = 0; k < K - 1; k++)
        ok &= (viz[k] == cols[k]);
    CHECK(ok);
    CHECK(vizinhos(g, 128, viz, 8) == 0);   /* linha vizinha intacta */

    graph_destroy(g);
}

/* O mesmo grafo nas duas representações: mesmos vizinhos, na mesma ordem
 * (é o que o #12 exige), e a matriz ocupando mais memória que a lista. */
static void lista_e_matriz_identicas(void)
{
    EdgeList el = arestas("10 20\n20 10\n20 30\n30 40\n40 10\n50 60\n60 70\n70 50\n80 10\n");
    for (int visao = GRAPH_DIRECTED; visao <= GRAPH_RECIPROCAL; visao++) {
        IdMap ids;
        idmap_iniciar(&ids);
        Graph *l, *m;
        CHECK(graph_build(&el, &ids, GRAPH_LIST, (GraphView)visao, &l) == GRAPH_OK);
        size_t antes = mt_current_bytes();
        CHECK(graph_build(&el, &ids, GRAPH_MATRIX, (GraphView)visao, &m) == GRAPH_OK);
        size_t bytes_matriz = mt_current_bytes() - antes;

        CHECK(graph_num_vertices(l) == graph_num_vertices(m));
        CHECK(graph_num_edges(l) == graph_num_edges(m));
        int iguais = 1;
        for (size_t v = 0; v < graph_num_vertices(l); v++) {
            size_t vl[16], vm[16];
            size_t kl = vizinhos(l, v, vl, 16), km = vizinhos(m, v, vm, 16);
            iguais &= (kl == km && graph_degree(l, v) == graph_degree(m, v));
            for (size_t k = 0; iguais && k < kl; k++)
                iguais &= (vl[k] == vm[k]);
        }
        CHECK(iguais);
        CHECK(bytes_matriz > 0);

        graph_destroy(l);
        graph_destroy(m);
        idmap_liberar(&ids);
    }
    edgelist_liberar(&el);
}

/* Em grafo esparso a matriz de bits ocupa mais que a lista, e cresce com V². */
static void memoria_lista_x_matriz(void)
{
    enum { N = 2000 };
    size_t bytes[2];
    for (int r = GRAPH_LIST; r <= GRAPH_MATRIX; r++) {
        size_t antes = mt_current_bytes();
        Graph *g;
        CHECK(graph_create(N, (GraphRep)r, GRAPH_DIRECTED, &g) == GRAPH_OK);
        for (size_t v = 0; v + 1 < N; v++)   /* caminho: grau 1 */
            CHECK(graph_add_edge(g, v, v + 1) == GRAPH_OK);
        bytes[r] = mt_current_bytes() - antes;
        graph_destroy(g);
    }
    /* N² bits = 500 KB, contra poucas dezenas de KB da lista */
    CHECK(bytes[GRAPH_MATRIX] >= (size_t)N * N / 8);
    CHECK(bytes[GRAPH_LIST] < bytes[GRAPH_MATRIX]);
}

static void casos_limite(void)
{
    Graph *g;
    graph_destroy(NULL);

    for (int r = GRAPH_LIST; r <= GRAPH_MATRIX; r++) {
        CHECK(graph_create(0, (GraphRep)r, GRAPH_DIRECTED, &g) == GRAPH_OK);
        CHECK(graph_num_vertices(g) == 0 && graph_num_edges(g) == 0);
        graph_destroy(g);
    }

    /* um único vértice: o iterador não devolve nada */
    CHECK(graph_create(1, GRAPH_MATRIX, GRAPH_DIRECTED, &g) == GRAPH_OK);
    size_t viz[1];
    CHECK(vizinhos(g, 0, viz, 1) == 0);
    graph_destroy(g);

    CHECK(strcmp(graph_rep_name(GRAPH_LIST), "lista") == 0);
    CHECK(strcmp(graph_rep_name(GRAPH_MATRIX), "matriz") == 0);
    CHECK(strcmp(graph_view_name(GRAPH_SYMMETRIC), "simetrizada") == 0);
}

int main(void)
{
    for (int r = GRAPH_LIST; r <= GRAPH_MATRIX; r++) {
        insercao_e_vizinhos((GraphRep)r);
        estrela_grande((GraphRep)r);
        tres_visoes((GraphRep)r);
        reciproca_vazia((GraphRep)r);
    }
    matriz_bordas_de_palavra();
    lista_e_matriz_identicas();
    memoria_lista_x_matriz();
    casos_limite();

    /* tudo o que o grafo alocou foi devolvido */
    CHECK(mt_current_bytes() == 0);

    remove(FIXTURE);
    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
