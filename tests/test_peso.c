/*
 * Testes das arestas com peso no TAD Grafo (#64), nas duas representações:
 * lista (src/graph_list.c) e matriz (src/graph_matrix.c).
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_peso && ./bin/test_peso
 */
#include "graph.h"
#include "memtrack.h"

#include <math.h>
#include <stdio.h>

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

/* Vizinhos de v com os pesos, na ordem do iterador. Devolve quantos são. */
static size_t vizinhos(const Graph *g, size_t v, size_t *viz, float *pesos, size_t max)
{
    GraphIter it;
    size_t w, k = 0;
    float p;
    graph_neighbors_begin(g, v, &it);
    while (graph_neighbors_next_weighted(&it, &w, &p)) {
        if (k < max) {
            viz[k] = w;
            pesos[k] = p;
        }
        k++;
    }
    return k;
}

/* Inserção fora de ordem: o iterador devolve os vizinhos ordenados, cada um
 * com o seu peso, e a consulta devolve o peso de u -> v. */
static void direcionado(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create_weighted(5, rep, GRAPH_DIRECTED, &g) == GRAPH_OK);
    CHECK(graph_is_weighted(g));
    CHECK(graph_add_weighted_edge(g, 0, 3, 2.5f) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 0, 1, 0.5f) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 0, 4, 0.0f) == GRAPH_OK);   /* peso zero vale */
    CHECK(graph_add_weighted_edge(g, 2, 0, 0.125f) == GRAPH_OK);
    CHECK(graph_num_edges(g) == 4);
    CHECK(graph_degree(g, 0) == 3);

    size_t viz[8];
    float p[8];
    CHECK(vizinhos(g, 0, viz, p, 8) == 3);
    CHECK(viz[0] == 1 && p[0] == 0.5f);
    CHECK(viz[1] == 3 && p[1] == 2.5f);
    CHECK(viz[2] == 4 && p[2] == 0.0f);

    CHECK(graph_edge_weight(g, 0, 3) == 2.5f);
    CHECK(graph_edge_weight(g, 2, 0) == 0.125f);
    CHECK(isinf(graph_edge_weight(g, 3, 0)));   /* o sentido importa */
    CHECK(isinf(graph_edge_weight(g, 1, 2)));
    CHECK(graph_has_edge(g, 0, 4) && !graph_has_edge(g, 4, 0));

    /* o iterador comum continua funcionando num grafo com peso */
    GraphIter it;
    size_t w, k = 0;
    graph_neighbors_begin(g, 0, &it);
    while (graph_neighbors_next(&it, &w))
        k++;
    CHECK(k == 3);
    graph_destroy(g);
}

/* Nas visões sem sentido, {u, v} tem um peso só, visto das duas pontas. */
static void sem_sentido(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create_weighted(4, rep, GRAPH_SYMMETRIC, &g) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 2, 1, 0.25f) == GRAPH_OK);
    CHECK(graph_num_edges(g) == 1);
    CHECK(graph_edge_weight(g, 1, 2) == 0.25f);
    CHECK(graph_edge_weight(g, 2, 1) == 0.25f);

    size_t viz[4];
    float p[4];
    CHECK(vizinhos(g, 1, viz, p, 4) == 1 && viz[0] == 2 && p[0] == 0.25f);
    CHECK(vizinhos(g, 2, viz, p, 4) == 1 && viz[0] == 1 && p[0] == 0.25f);
    graph_destroy(g);
}

/* Inserir de novo uma aresta que já existe troca o peso e não conta outra
 * aresta; nas visões sem sentido, troca nas duas pontas. */
static void reinsercao_troca_o_peso(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create_weighted(3, rep, GRAPH_DIRECTED, &g) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 0, 1, 4.0f) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 0, 1, 0.75f) == GRAPH_OK);
    CHECK(graph_num_edges(g) == 1);
    CHECK(graph_degree(g, 0) == 1);
    CHECK(graph_edge_weight(g, 0, 1) == 0.75f);
    graph_destroy(g);

    CHECK(graph_create_weighted(3, rep, GRAPH_RECIPROCAL, &g) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 0, 2, 4.0f) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 2, 0, 0.5f) == GRAPH_OK);
    CHECK(graph_num_edges(g) == 1);
    CHECK(graph_edge_weight(g, 0, 2) == 0.5f && graph_edge_weight(g, 2, 0) == 0.5f);
    graph_destroy(g);
}

/* graph_add_edge num grafo com peso insere com peso 1, e laços continuam
 * ignorados com ou sem peso. */
static void aresta_sem_peso_e_laco(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create_weighted(3, rep, GRAPH_DIRECTED, &g) == GRAPH_OK);
    CHECK(graph_add_edge(g, 1, 2) == GRAPH_OK);
    CHECK(graph_edge_weight(g, 1, 2) == 1.0f);
    CHECK(graph_add_weighted_edge(g, 1, 1, 3.0f) == GRAPH_OK);
    CHECK(graph_num_edges(g) == 1);
    CHECK(isinf(graph_edge_weight(g, 1, 1)));
    graph_destroy(g);
}

/* O grafo da Fase I, sem peso, responde como se todo peso fosse 1, e
 * recusa uma aresta com peso em vez de jogar o peso fora. */
static void grafo_sem_peso(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create(3, rep, GRAPH_DIRECTED, &g) == GRAPH_OK);
    CHECK(!graph_is_weighted(g));
    CHECK(graph_add_edge(g, 0, 2) == GRAPH_OK);
    CHECK(graph_edge_weight(g, 0, 2) == 1.0f);
    CHECK(isinf(graph_edge_weight(g, 2, 0)));

    size_t viz[3];
    float p[3];
    CHECK(vizinhos(g, 0, viz, p, 3) == 1 && viz[0] == 2 && p[0] == 1.0f);

    CHECK(graph_add_weighted_edge(g, 1, 2, 0.5f) == GRAPH_ERR_NOT_WEIGHTED);
    CHECK(!graph_has_edge(g, 1, 2));
    CHECK(graph_num_edges(g) == 1);
    graph_destroy(g);
}

/* Peso tem que ser um número finito: NaN e infinito são recusados sem
 * inserir nada. Peso negativo é aceito pelo TAD (quem o recusa é o Dijkstra). */
static void peso_invalido(GraphRep rep)
{
    Graph *g;
    CHECK(graph_create_weighted(3, rep, GRAPH_DIRECTED, &g) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 0, 1, NAN) == GRAPH_ERR_INVALID_WEIGHT);
    CHECK(graph_add_weighted_edge(g, 0, 1, INFINITY) == GRAPH_ERR_INVALID_WEIGHT);
    CHECK(graph_add_weighted_edge(g, 0, 1, -INFINITY) == GRAPH_ERR_INVALID_WEIGHT);
    CHECK(graph_num_edges(g) == 0 && !graph_has_edge(g, 0, 1));

    CHECK(graph_add_weighted_edge(g, 0, 1, 2.0f) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 0, 1, NAN) == GRAPH_ERR_INVALID_WEIGHT);
    CHECK(graph_edge_weight(g, 0, 1) == 2.0f);   /* o peso antigo fica */

    CHECK(graph_add_weighted_edge(g, 1, 2, -1.5f) == GRAPH_OK);
    CHECK(graph_edge_weight(g, 1, 2) == -1.5f);
    graph_destroy(g);
}

/* Na matriz, as colunas 63, 64 e 127 ficam nas bordas das palavras de 64 bits
 * que dizem quais arestas existem. */
static void matriz_bordas_de_palavra(void)
{
    Graph *g;
    CHECK(graph_create_weighted(130, GRAPH_MATRIX, GRAPH_DIRECTED, &g) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 5, 127, 3.0f) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 5, 63, 1.0f) == GRAPH_OK);
    CHECK(graph_add_weighted_edge(g, 5, 64, 2.0f) == GRAPH_OK);
    size_t viz[4];
    float p[4];
    CHECK(vizinhos(g, 5, viz, p, 4) == 3);
    CHECK(viz[0] == 63 && p[0] == 1.0f);
    CHECK(viz[1] == 64 && p[1] == 2.0f);
    CHECK(viz[2] == 127 && p[2] == 3.0f);
    graph_destroy(g);
}

/* Gerador congruencial fixo: o mesmo grafo em toda execução e plataforma. */
static unsigned long proximo(unsigned long *s)
{
    *s = (*s * 1103515245UL + 12345UL) & 0x7fffffffUL;
    return *s;
}

/* O mesmo grafo aleatório com peso, montado nas duas representações, tem as
 * mesmas arestas, os mesmos pesos e o mesmo percurso, vizinho a vizinho. */
static void lista_e_matriz_identicas(GraphView view)
{
    const size_t n = 200;
    Graph *l, *m;
    CHECK(graph_create_weighted(n, GRAPH_LIST, view, &l) == GRAPH_OK);
    CHECK(graph_create_weighted(n, GRAPH_MATRIX, view, &m) == GRAPH_OK);

    unsigned long s = 2026;
    for (int i = 0; i < 3000; i++) {
        size_t u = proximo(&s) % n, v = proximo(&s) % n;
        float p = (float)(proximo(&s) % 1000) / 1000.0f;   /* repete pares: troca o peso */
        CHECK(graph_add_weighted_edge(l, u, v, p) == GRAPH_OK);
        CHECK(graph_add_weighted_edge(m, u, v, p) == GRAPH_OK);
    }
    CHECK(graph_num_edges(l) == graph_num_edges(m));

    int iguais = 1;
    for (size_t u = 0; u < n && iguais; u++) {
        GraphIter a, b;
        size_t wa, wb;
        float pa, pb;
        graph_neighbors_begin(l, u, &a);
        graph_neighbors_begin(m, u, &b);
        int ra, rb;
        do {
            ra = graph_neighbors_next_weighted(&a, &wa, &pa);
            rb = graph_neighbors_next_weighted(&b, &wb, &pb);
            if (ra != rb || (ra && (wa != wb || pa != pb ||
                                    graph_edge_weight(l, u, wa) != pa ||
                                    graph_edge_weight(m, u, wb) != pb)))
                iguais = 0;
        } while (ra && rb && iguais);
        for (size_t v = 0; v < n && iguais; v++) {
            float x = graph_edge_weight(l, u, v), y = graph_edge_weight(m, u, v);
            if (!(x == y || (isinf(x) && isinf(y))))
                iguais = 0;
        }
    }
    CHECK(iguais);
    graph_destroy(l);
    graph_destroy(m);
}

/* A matriz com peso guarda um float por par, além dos bits; a lista, um
 * float por aresta além do índice. E tudo volta ao memtrack no destroy. */
static void memoria(void)
{
    const size_t n = 1000;
    size_t antes = mt_current_bytes();
    Graph *g;
    CHECK(graph_create_weighted(n, GRAPH_MATRIX, GRAPH_DIRECTED, &g) == GRAPH_OK);
    size_t com_peso = mt_current_bytes() - antes;
    graph_destroy(g);

    CHECK(graph_create(n, GRAPH_MATRIX, GRAPH_DIRECTED, &g) == GRAPH_OK);
    size_t sem_peso = mt_current_bytes() - antes;
    graph_destroy(g);

    CHECK(com_peso >= sem_peso + n * n * sizeof(float));
    CHECK(com_peso < sem_peso + n * n * sizeof(float) + 64 * 1024);

    CHECK(graph_create_weighted(n, GRAPH_LIST, GRAPH_DIRECTED, &g) == GRAPH_OK);
    for (size_t v = 1; v < n; v++)
        graph_add_weighted_edge(g, 0, v, 1.0f);
    graph_destroy(g);
    CHECK(mt_current_bytes() == antes);
}

static void rodar(GraphRep rep)
{
    direcionado(rep);
    sem_sentido(rep);
    reinsercao_troca_o_peso(rep);
    aresta_sem_peso_e_laco(rep);
    grafo_sem_peso(rep);
    peso_invalido(rep);
}

int main(void)
{
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);
    matriz_bordas_de_palavra();
    lista_e_matriz_identicas(GRAPH_DIRECTED);
    lista_e_matriz_identicas(GRAPH_SYMMETRIC);
    memoria();
    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
