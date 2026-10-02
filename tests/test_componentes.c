/*
 * Testes dos componentes conexos (src/componentes.c), nas duas representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_componentes && ./bin/test_componentes
 */
#include "componentes.h"
#include "memtrack.h"

#include <stdint.h>
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

static int juntos(const ComponentesResultado *r, size_t u, size_t v)
{
    return r->componente[u] == r->componente[v];
}

/* Invariantes de qualquer resultado: rótulos válidos, tamanhos que batem com
 * os rótulos e somam V, e gigante/unitários coerentes com os tamanhos. */
static int consistente(const ComponentesResultado *r)
{
    size_t *conta = calloc(r->num_componentes + 1, sizeof *conta);
    int ok = conta != NULL;
    for (size_t v = 0; ok && v < r->n; v++) {
        ok = r->componente[v] < r->num_componentes;
        if (ok)
            conta[r->componente[v]]++;
    }
    size_t soma = 0, unit = 0, maior = 0;
    for (size_t c = 0; ok && c < r->num_componentes; c++) {
        ok = conta[c] == r->tamanho[c] && conta[c] > 0;
        soma += conta[c];
        unit += conta[c] == 1;
        if (conta[c] > maior)
            maior = conta[c];
    }
    free(conta);
    return ok && soma == r->n && unit == r->unitarios && maior == r->tamanho_gigante &&
           (r->n == 0 || r->tamanho[r->gigante] == maior);
}

/* 0 -> 1 -> 2 -> 0 e 2 -> 3: o ciclo é um componente; o 3 alcança ninguém. */
static void ciclo_com_cauda(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 4, a);
    ComponentesResultado r;
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(consistente(&r));
    CHECK(r.num_componentes == 2);
    CHECK(juntos(&r, 0, 1) && juntos(&r, 1, 2));
    CHECK(!juntos(&r, 2, 3));
    CHECK(r.tamanho_gigante == 3 && r.componente[0] == r.gigante);
    CHECK(r.unitarios == 1);
    componentes_liberar(&r);
    graph_destroy(g);
}

/* 0 -> 1 -> 2: sem volta, cada vértice é um componente. */
static void caminho_sem_volta(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 3, a);
    ComponentesResultado r;
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(consistente(&r));
    CHECK(r.num_componentes == 3);
    CHECK(r.unitarios == 3);
    CHECK(r.tamanho_gigante == 1);
    componentes_liberar(&r);
    graph_destroy(g);
}

/* 0 <-> 1 e 2 <-> 3, com 1 -> 2: a ponte num sentido só não junta os pares. */
static void pares_ligados_num_sentido(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 0}, {2, 3}, {3, 2}, {1, 2}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 4, a);
    ComponentesResultado r;
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(consistente(&r));
    CHECK(r.num_componentes == 2);
    CHECK(juntos(&r, 0, 1) && juntos(&r, 2, 3));
    CHECK(!juntos(&r, 1, 2));
    CHECK(r.unitarios == 0);
    componentes_liberar(&r);
    graph_destroy(g);
}

/* Cormen et al., Introduction to Algorithms, figura 22.9 (componentes
 * fortemente conexos), com a..h = 0..7. Resposta do livro:
 * {a, b, e}, {c, d}, {f, g}, {h}. O laço h -> h é ignorado pelo grafo. */
static void exemplo_do_cormen(GraphRep rep)
{
    enum { A, B, C, D, E, F, G, H };
    Par a[] = {{A, B}, {B, C}, {B, E}, {B, F}, {C, D}, {C, G}, {D, C}, {D, H},
               {E, A}, {E, F}, {F, G}, {G, F}, {G, H}, {H, H}};
    Graph *g = MONTAR(rep, GRAPH_DIRECTED, 8, a);
    ComponentesResultado r;
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(consistente(&r));
    CHECK(r.num_componentes == 4);
    CHECK(juntos(&r, A, B) && juntos(&r, B, E));
    CHECK(juntos(&r, C, D));
    CHECK(juntos(&r, F, G));
    CHECK(!juntos(&r, A, C) && !juntos(&r, C, F) && !juntos(&r, F, H) && !juntos(&r, A, H));
    CHECK(r.tamanho_gigante == 3 && r.componente[A] == r.gigante);
    CHECK(r.unitarios == 1 && r.tamanho[r.componente[H]] == 1);
    componentes_liberar(&r);
    graph_destroy(g);
}

/* Na visão simetrizada, as mesmas arestas viram componentes fracos: o
 * caminho sem volta, que dava 3 componentes fortes, é um componente só. */
static void simetrizada_da_fracamente_conexos(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {4, 5}};   /* 3 sem nenhuma aresta */
    Graph *g = MONTAR(rep, GRAPH_SYMMETRIC, 6, a);
    ComponentesResultado r;
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(consistente(&r));
    CHECK(r.num_componentes == 3);
    CHECK(juntos(&r, 0, 1) && juntos(&r, 1, 2) && juntos(&r, 4, 5));
    CHECK(!juntos(&r, 2, 3) && !juntos(&r, 3, 4));
    CHECK(r.tamanho_gigante == 3 && r.componente[0] == r.gigante);
    CHECK(r.unitarios == 1 && r.tamanho[r.componente[3]] == 1);   /* o isolado */
    componentes_liberar(&r);
    graph_destroy(g);
}

/* {0, 1} e {2, 3}, do mesmo tamanho: o gigante é o de menor número, o
 * primeiro que o Tarjan fecha, que é o do vértice 0. */
static void empate_no_gigante(GraphRep rep)
{
    Par a[] = {{0, 1}, {2, 3}};
    Graph *g = MONTAR(rep, GRAPH_SYMMETRIC, 4, a);
    ComponentesResultado r;
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(r.num_componentes == 2 && r.tamanho_gigante == 2);
    CHECK(r.gigante == 0 && r.componente[0] == r.gigante);
    componentes_liberar(&r);
    graph_destroy(g);
}

static void vazio_e_um_vertice(GraphRep rep)
{
    Graph *g;
    ComponentesResultado r;

    graph_create(0, rep, GRAPH_DIRECTED, &g);
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(r.num_componentes == 0 && r.tamanho_gigante == 0 && r.unitarios == 0);
    CHECK(r.gigante == SIZE_MAX);
    componentes_liberar(&r);
    graph_destroy(g);

    graph_create(1, rep, GRAPH_DIRECTED, &g);
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(consistente(&r));
    CHECK(r.num_componentes == 1 && r.tamanho_gigante == 1 && r.unitarios == 1);
    componentes_liberar(&r);
    graph_destroy(g);
}

/* Ciclo 0 -> 1 -> ... -> n-1 -> 0: a DFS desce n níveis antes de voltar.
 * Com recursão, isso estouraria a pilha de chamadas. */
static void ciclo_profundo(GraphRep rep, size_t n)
{
    Graph *g;
    graph_create(n, rep, GRAPH_DIRECTED, &g);
    for (size_t v = 0; v < n; v++)
        graph_add_edge(g, v, (v + 1) % n);
    ComponentesResultado r;
    CHECK(componentes_executar(g, &r) == COMPONENTES_OK);
    CHECK(r.num_componentes == 1 && r.tamanho_gigante == n && r.unitarios == 0);
    componentes_liberar(&r);
    graph_destroy(g);
}

/* Componentes de tamanhos 3, 1, 1, 2, 2, 2: a distribuição agrupa por
 * tamanho, do maior para o menor. */
static void distribuicao_de_tamanhos(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {5, 6}, {7, 8}, {9, 10}};   /* 3 e 4 isolados */
    Graph *g = MONTAR(rep, GRAPH_SYMMETRIC, 11, a);
    ComponentesResultado r;
    componentes_executar(g, &r);

    ComponentesFaixa *f;
    size_t k;
    CHECK(componentes_distribuicao(&r, &f, &k) == COMPONENTES_OK);
    CHECK(k == 3);
    CHECK(k == 3 && f[0].tamanho == 3 && f[0].quantidade == 1);
    CHECK(k == 3 && f[1].tamanho == 2 && f[1].quantidade == 3);
    CHECK(k == 3 && f[2].tamanho == 1 && f[2].quantidade == 2);
    mt_free(f);
    componentes_liberar(&r);
    graph_destroy(g);
}

static void distribuicao_vazia(GraphRep rep)
{
    Graph *g;
    graph_create(0, rep, GRAPH_DIRECTED, &g);
    ComponentesResultado r;
    componentes_executar(g, &r);
    ComponentesFaixa *f;
    size_t k = 99;
    CHECK(componentes_distribuicao(&r, &f, &k) == COMPONENTES_OK);
    CHECK(k == 0);
    mt_free(f);
    componentes_liberar(&r);
    graph_destroy(g);
}

static void rodar(GraphRep rep)
{
    distribuicao_de_tamanhos(rep);
    distribuicao_vazia(rep);
    ciclo_com_cauda(rep);
    caminho_sem_volta(rep);
    pares_ligados_num_sentido(rep);
    exemplo_do_cormen(rep);
    simetrizada_da_fracamente_conexos(rep);
    empate_no_gigante(rep);
    vazio_e_um_vertice(rep);
}

int main(void)
{
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);
    ciclo_profundo(GRAPH_LIST, 100000);
    ciclo_profundo(GRAPH_MATRIX, 20000);   /* 20.000² bits = 50 MB */

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
