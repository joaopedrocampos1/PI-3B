/*
 * Testes da amostragem de subgrafos (src/subgraph.c).
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   gcc -std=c11 -Wall -Wextra -Iinclude src/subgraph.c src/idmap.c src/edgelist.c \
 *       tests/test_subgraph.c -o bin/test_subgraph
 *   ./bin/test_subgraph
 */
#include "subgraph.h"

#include <stdio.h>
#include <stdlib.h>

#define FIXTURE "bin/fixture_subgraph.txt"

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
static EdgeList grafo(const char *conteudo)
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
        fprintf(stderr, "fixture ilegível\n");
        exit(2);
    }
    return el;
}

static int tem_aresta(const EdgeList *el, unsigned long long o, unsigned long long d)
{
    for (size_t i = 0; i < el->n; i++)
        if (el->arestas[i].origem == o && el->arestas[i].destino == d)
            return 1;
    return 0;
}

/* Caminho 1->2->3->4->5. A partir do 3, o 2 só é alcançado andando contra o
 * sentido de 2->3. Ordem de descoberta: 3, 2, 4. */
static void bfs_ignora_sentido_e_para_em_n(void)
{
    EdgeList el = grafo("1 2\n2 3\n3 4\n4 5\n");
    Amostrador a;
    CHECK(subgraph_preparar(&a, &el) == AMOSTRA_OK);

    EdgeList sub;
    CHECK(subgraph_bfs(&a, 3, 3, &sub) == AMOSTRA_OK);
    CHECK(sub.n == 2);
    CHECK(tem_aresta(&sub, 2, 3));
    CHECK(tem_aresta(&sub, 3, 4));
    CHECK(!tem_aresta(&sub, 1, 2));
    CHECK(!tem_aresta(&sub, 4, 5));

    edgelist_liberar(&sub);
    subgraph_liberar(&a);
    edgelist_liberar(&el);
}

/* Arestas lidas (já ordenadas): 1-2, 1-3, 1-5, 2-1, 2-3, 4-1, 5-6.
 * Índices por ordem de aparição: 1:0, 2:1, 3:2, 5:3, 4:4, 6:5.
 * Do 1, com N=3, a amostra é {1, 2, 3}. */
static void subgrafo_induzido_completo(void)
{
    EdgeList el = grafo("1 2\n2 1\n1 3\n2 3\n4 1\n1 5\n5 6\n");
    Amostrador a;
    subgraph_preparar(&a, &el);

    EdgeList sub;
    CHECK(subgraph_bfs(&a, 1, 3, &sub) == AMOSTRA_OK);
    CHECK(sub.n == 4);
    CHECK(tem_aresta(&sub, 1, 2));
    CHECK(tem_aresta(&sub, 2, 1));   /* recíproca */
    CHECK(tem_aresta(&sub, 1, 3));
    CHECK(tem_aresta(&sub, 2, 3));   /* entre amostrados, fora da árvore do BFS */
    edgelist_liberar(&sub);

    /* N=4: o 4º vértice é o 5 (índice 3), não o 4 (índice 4) */
    CHECK(subgraph_bfs(&a, 1, 4, &sub) == AMOSTRA_OK);
    CHECK(sub.n == 5);
    CHECK(tem_aresta(&sub, 1, 5));
    CHECK(!tem_aresta(&sub, 4, 1));
    edgelist_liberar(&sub);

    subgraph_liberar(&a);
    edgelist_liberar(&el);
}

/* Arestas: 1-7, 5-6, 7-5. Índices: 1:0, 7:1, 5:2, 6:3.
 * Vizinhos do 5: o 6 aparece antes na leitura, mas o 7 tem índice menor.
 * Visitar em ordem de índice dá {5, 7}; em ordem de leitura daria {5, 6}. */
static void vizinhos_em_ordem_de_indice(void)
{
    EdgeList el = grafo("1 7\n5 6\n7 5\n");
    Amostrador a;
    subgraph_preparar(&a, &el);

    EdgeList sub;
    CHECK(subgraph_bfs(&a, 5, 2, &sub) == AMOSTRA_OK);
    CHECK(sub.n == 1);
    CHECK(tem_aresta(&sub, 7, 5));
    edgelist_liberar(&sub);

    subgraph_liberar(&a);
    edgelist_liberar(&el);
}

static void erros_e_limites(void)
{
    EdgeList el = grafo("1 2\n3 4\n4 5\n");   /* componentes {1,2} e {3,4,5} */
    Amostrador a;
    subgraph_preparar(&a, &el);
    EdgeList sub;

    CHECK(subgraph_bfs(&a, 99, 2, &sub) == AMOSTRA_SEMENTE_INEXISTENTE);
    CHECK(sub.n == 0 && sub.arestas == NULL);

    CHECK(subgraph_bfs(&a, 1, 3, &sub) == AMOSTRA_COMPONENTE_PEQUENO);
    CHECK(sub.n == 0 && sub.arestas == NULL);

    CHECK(subgraph_bfs(&a, 1, 2, &sub) == AMOSTRA_OK);   /* N = tamanho do componente */
    CHECK(sub.n == 1 && tem_aresta(&sub, 1, 2));
    edgelist_liberar(&sub);

    CHECK(subgraph_bfs(&a, 3, 1, &sub) == AMOSTRA_OK);   /* só a semente: nenhuma aresta */
    CHECK(sub.n == 0);
    edgelist_liberar(&sub);

    CHECK(subgraph_bfs(&a, 3, 0, &sub) == AMOSTRA_OK);
    CHECK(sub.n == 0);
    edgelist_liberar(&sub);

    subgraph_liberar(&a);
    edgelist_liberar(&el);
}

/* Caminho com 1.000 vértices, IDs 1000..1999. */
static EdgeList caminho_mil(void)
{
    FILE *f = fopen(FIXTURE, "wb");
    for (int i = 1000; i < 1999; i++)
        fprintf(f, "%d %d\n", i, i + 1);
    fclose(f);
    EdgeList el;
    edgelist_ler(FIXTURE, &el);
    return el;
}

static int distintas_e_existentes(const Amostrador *a, const unsigned long long *s, size_t k)
{
    size_t idx;
    for (size_t i = 0; i < k; i++) {
        if (!idmap_buscar(&a->ids, s[i], &idx))
            return 0;
        for (size_t j = 0; j < i; j++)
            if (s[i] == s[j])
                return 0;
    }
    return 1;
}

static void sorteio_de_sementes(void)
{
    EdgeList el = caminho_mil();
    Amostrador a;
    subgraph_preparar(&a, &el);
    unsigned long long s1[3], s2[3], s3[3];

    CHECK(subgraph_sortear_sementes(&a, 42, 3, s1) == AMOSTRA_OK);
    CHECK(distintas_e_existentes(&a, s1, 3));

    CHECK(subgraph_sortear_sementes(&a, 42, 3, s2) == AMOSTRA_OK);
    CHECK(s1[0] == s2[0] && s1[1] == s2[1] && s1[2] == s2[2]);   /* reproduzível */

    CHECK(subgraph_sortear_sementes(&a, 7, 3, s3) == AMOSTRA_OK);
    CHECK(!(s1[0] == s3[0] && s1[1] == s3[1] && s1[2] == s3[2])); /* depende da semente */

    subgraph_liberar(&a);
    edgelist_liberar(&el);
}

/* k = V obriga o sorteio a usar todos os vértices; k > V é impossível. */
static void sorteio_nos_limites(void)
{
    EdgeList el = grafo("1 2\n2 3\n");
    Amostrador a;
    subgraph_preparar(&a, &el);
    unsigned long long s[4];

    CHECK(subgraph_sortear_sementes(&a, 1, 3, s) == AMOSTRA_OK);
    CHECK(distintas_e_existentes(&a, s, 3));
    CHECK(subgraph_sortear_sementes(&a, 1, 4, s) == AMOSTRA_SEMENTES_INSUFICIENTES);

    subgraph_liberar(&a);
    edgelist_liberar(&el);
}

int main(void)
{
    sorteio_de_sementes();
    sorteio_nos_limites();
    erros_e_limites();
    subgrafo_induzido_completo();
    vizinhos_em_ordem_de_indice();
    bfs_ignora_sentido_e_para_em_n();

    remove(FIXTURE);
    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
