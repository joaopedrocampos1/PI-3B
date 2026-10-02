/*
 * Testes dos graus de separação (src/separacao.c), nas duas representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_separacao && ./bin/test_separacao
 */
#include "separacao.h"
#include "memtrack.h"

#include <math.h>
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

#define PERTO(a, b) (fabs((a) - (b)) < 1e-9)

typedef struct {
    size_t u, v;
} Par;

static Graph *montar(GraphRep rep, size_t n, const Par *a, size_t m)
{
    Graph *g;
    if (graph_create(n, rep, GRAPH_DIRECTED, &g) != GRAPH_OK) {
        fprintf(stderr, "sem memória para o grafo de teste\n");
        exit(2);
    }
    for (size_t i = 0; i < m; i++)
        graph_add_edge(g, a[i].u, a[i].v);
    return g;
}

#define MONTAR(rep, n, arestas) montar(rep, n, arestas, sizeof arestas / sizeof *arestas)

/* Os perfis 1..8 seguem o influenciador 0; o perfil 9 segue o 1.
 * Influenciador: só o 0 (8 seguidores). Do 9 até o 0 são 2 saltos. */
static void um_influenciador(GraphRep rep)
{
    Par a[] = {{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 1}};
    Graph *g = MONTAR(rep, 10, a);

    size_t grau[10];
    separacao_grau_entrada(g, grau);
    CHECK(grau[0] == 8 && grau[1] == 1 && grau[9] == 0);

    SeparacaoResultado r;
    CHECK(separacao_analisar(g, 0.1, 100, 7, &r) == SEPARACAO_OK);
    CHECK(r.num_influenciadores == 1 && r.eh_influenciador[0]);
    CHECK(r.grau_entrada_corte == 8);
    CHECK(r.num_perfis == 9);   /* pediu 100, só há 9 perfis comuns */

    int ordem = 1;
    for (size_t i = 0; i < r.num_perfis; i++)
        ordem &= (r.perfis[i].vertice == i + 1);   /* todos, em ordem crescente */
    CHECK(ordem);

    CHECK(r.perfis[0].dist_influenciador == 1);
    CHECK(r.perfis[8].dist_influenciador == 2);   /* perfil 9 */
    CHECK(r.perfis[8].alcancados == 3 && r.perfis[8].excentricidade == 2);
    CHECK(r.perfis_sem_influenciador == 0);
    CHECK(PERTO(r.dist_media_influenciador, 10.0 / 9.0));
    CHECK(PERTO(r.dist_media_conjunto, 10.0 / 9.0));
    /* pares: 8 perfis a 1 salto do 0, e o 9 a 1 do 1 e a 2 do 0 */
    CHECK(r.hist_rede[1] == 9 && r.hist_rede[2] == 1);
    CHECK(PERTO(r.dist_media_rede, 11.0 / 10.0));
    CHECK(r.hist_influenciador[1] == 8 && r.hist_influenciador[2] == 1);
    CHECK(r.diametro_estimado == 2);

    separacao_liberar(&r);
    graph_destroy(g);
}

/* Empate no corte: 0 e 1 têm 2 seguidores cada. Pedindo 1 influenciador
 * (25% de 4), entram os dois. */
static void empate_no_corte(GraphRep rep)
{
    Par a[] = {{2, 0}, {3, 0}, {2, 1}, {3, 1}};
    Graph *g = MONTAR(rep, 4, a);
    SeparacaoResultado r;
    CHECK(separacao_analisar(g, 0.25, 10, 1, &r) == SEPARACAO_OK);
    CHECK(r.num_influenciadores == 2);
    CHECK(r.eh_influenciador[0] && r.eh_influenciador[1]);
    CHECK(r.num_perfis == 2);
    CHECK(r.perfis[0].influenciadores_alcancados == 2);
    CHECK(PERTO(r.dist_media_influenciador, 1.0));
    separacao_liberar(&r);
    graph_destroy(g);
}

/* Perfis que não seguem ninguém não alcançam influenciador nenhum. */
static void sem_alcance(GraphRep rep)
{
    Par a[] = {{1, 0}, {2, 0}};
    Graph *g = MONTAR(rep, 5, a);   /* 3 e 4 isolados */
    SeparacaoResultado r;
    CHECK(separacao_analisar(g, 0.2, 10, 1, &r) == SEPARACAO_OK);
    CHECK(r.num_influenciadores == 1 && r.num_perfis == 4);
    CHECK(r.perfis_sem_influenciador == 2);
    CHECK(r.perfis[2].dist_influenciador == SIZE_MAX);   /* vértice 3 */
    CHECK(r.perfis[2].alcancados == 1);
    CHECK(PERTO(r.dist_media_influenciador, 1.0));   /* só entre os que alcançam */
    separacao_liberar(&r);
    graph_destroy(g);
}

/* O sorteio é reprodutível, sem repetição e só de perfis comuns. */
static void sorteio(GraphRep rep)
{
    enum { N = 300 };
    Par a[N];
    for (size_t i = 0; i < N; i++)
        a[i] = (Par){i, (i * 7 + 3) % 10};   /* 0..9 recebem todos os seguidores */
    Graph *g = montar(rep, N, a, N);

    SeparacaoResultado r1, r2, r3;
    CHECK(separacao_analisar(g, 0.01, 50, 42, &r1) == SEPARACAO_OK);
    CHECK(separacao_analisar(g, 0.01, 50, 42, &r2) == SEPARACAO_OK);
    CHECK(separacao_analisar(g, 0.01, 50, 43, &r3) == SEPARACAO_OK);
    CHECK(r1.num_perfis == 50);

    int iguais = 1, diferentes = 0, crescente = 1, comuns = 1;
    for (size_t i = 0; i < 50; i++) {
        iguais &= (r1.perfis[i].vertice == r2.perfis[i].vertice);
        diferentes |= (r1.perfis[i].vertice != r3.perfis[i].vertice);
        if (i > 0)
            crescente &= (r1.perfis[i - 1].vertice < r1.perfis[i].vertice);
        comuns &= !r1.eh_influenciador[r1.perfis[i].vertice];
    }
    CHECK(iguais);
    CHECK(diferentes);
    CHECK(crescente);   /* crescente estrito: sem repetição */
    CHECK(comuns);

    separacao_liberar(&r1);
    separacao_liberar(&r2);
    separacao_liberar(&r3);
    graph_destroy(g);
}

/* Caminho 0 -> 1 -> 2 -> 3: diâmetro 3, distância média 10/6. */
static void exato(GraphRep rep)
{
    Par a[] = {{0, 1}, {1, 2}, {2, 3}};
    Graph *g = MONTAR(rep, 4, a);
    size_t diam;
    double media;
    CHECK(separacao_exato(g, &diam, &media) == SEPARACAO_OK);
    CHECK(diam == 3);
    CHECK(PERTO(media, 10.0 / 6.0));
    graph_destroy(g);
}

static void erros(void)
{
    Par a[] = {{1, 0}};
    Graph *g = MONTAR(GRAPH_LIST, 2, a);
    SeparacaoResultado r;
    CHECK(separacao_analisar(g, 0.0, 10, 1, &r) == SEPARACAO_ERRO_PARAMETRO);
    CHECK(separacao_analisar(g, 1.5, 10, 1, &r) == SEPARACAO_ERRO_PARAMETRO);
    CHECK(separacao_analisar(g, 0.5, 0, 1, &r) == SEPARACAO_ERRO_PARAMETRO);
    CHECK(separacao_analisar(g, 1.0, 10, 1, &r) == SEPARACAO_ERRO_PARAMETRO);   /* todos influenciadores */
    separacao_liberar(&r);
    graph_destroy(g);
}

int main(void)
{
    for (int rep = GRAPH_LIST; rep <= GRAPH_MATRIX; rep++) {
        um_influenciador((GraphRep)rep);
        empate_no_corte((GraphRep)rep);
        sem_alcance((GraphRep)rep);
        sorteio((GraphRep)rep);
        exato((GraphRep)rep);
    }
    erros();

    CHECK(mt_current_bytes() == 0);

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
