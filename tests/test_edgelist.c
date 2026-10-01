/*
 * Testes do leitor de edge list (src/edgelist.c).
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   gcc -std=c11 -Wall -Wextra -Iinclude src/edgelist.c tests/test_edgelist.c -o bin/test_edgelist
 *   ./bin/test_edgelist
 */
#include "edgelist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIXTURE "bin/fixture_edgelist.txt"

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

/* Grava `conteudo` no arquivo temporário e devolve seu caminho. */
static const char *fixture(const char *conteudo)
{
    FILE *f = fopen(FIXTURE, "wb");
    if (!f) {
        perror(FIXTURE);
        exit(2);
    }
    fputs(conteudo, f);
    fclose(f);
    return FIXTURE;
}

static int tem_aresta(const EdgeList *el, unsigned long long o, unsigned long long d)
{
    for (size_t i = 0; i < el->n; i++)
        if (el->arestas[i].origem == o && el->arestas[i].destino == d)
            return 1;
    return 0;
}

static void le_pares_simples(void)
{
    EdgeList el;
    CHECK(edgelist_ler(fixture("1 2\n3 4\n"), &el) == EDGELIST_OK);
    CHECK(el.n == 2);
    CHECK(tem_aresta(&el, 1, 2));
    CHECK(tem_aresta(&el, 3, 4));
    CHECK(el.malformadas == 0);
    edgelist_liberar(&el);
}

static void ignora_comentarios_e_linhas_vazias(void)
{
    EdgeList el;
    CHECK(edgelist_ler(fixture("# Directed graph\n# FromNodeId ToNodeId\n\n1 2\n   \n# fim\n"),
                       &el) == EDGELIST_OK);
    CHECK(el.n == 1);
    CHECK(tem_aresta(&el, 1, 2));
    CHECK(el.malformadas == 0);
    edgelist_liberar(&el);
}

static void aceita_separadores_variados(void)
{
    EdgeList el;
    CHECK(edgelist_ler(fixture("1\t2\n3,4\n5;6\n7 ,  8\n9 10\r\n"), &el) == EDGELIST_OK);
    CHECK(el.n == 5);
    CHECK(tem_aresta(&el, 1, 2));
    CHECK(tem_aresta(&el, 3, 4));
    CHECK(tem_aresta(&el, 5, 6));
    CHECK(tem_aresta(&el, 7, 8));
    CHECK(tem_aresta(&el, 9, 10));
    CHECK(el.malformadas == 0);
    edgelist_liberar(&el);
}

static void descarta_linhas_malformadas(void)
{
    static const char *casos[] = {
        "5",                          /* um campo só */
        "a b",                        /* não numérico */
        "-1 2",                       /* negativo (strtoull aceitaria e daria o complemento) */
        "+1 2",                       /* sinal explícito */
        "3 4x",                       /* lixo colado ao número */
        "1 2 3",                      /* campo a mais */
        "99999999999999999999999 1",  /* estoura unsigned long long */
    };

    for (size_t i = 0; i < sizeof casos / sizeof casos[0]; i++) {
        char conteudo[128];
        snprintf(conteudo, sizeof conteudo, "1 2\n%s\n6 7\n", casos[i]);

        EdgeList el;
        CHECK(edgelist_ler(fixture(conteudo), &el) == EDGELIST_OK);
        if (el.n != 2 || el.malformadas != 1 || el.primeira_malformada != 2)
            fprintf(stderr, "  caso malformado: \"%s\"\n", casos[i]);
        CHECK(el.n == 2);
        CHECK(tem_aresta(&el, 1, 2));
        CHECK(tem_aresta(&el, 6, 7));
        CHECK(el.malformadas == 1);
        CHECK(el.primeira_malformada == 2);
        edgelist_liberar(&el);
    }
}

static void sem_malformadas_primeira_e_zero(void)
{
    EdgeList el;
    CHECK(edgelist_ler(fixture("1 2\n"), &el) == EDGELIST_OK);
    CHECK(el.primeira_malformada == 0);
    edgelist_liberar(&el);
}

/* Monta "1 2\n" + prefixo + 3000 espaços + "8 9\n" + "6 7\n": uma linha bem
 * maior que qualquer buffer razoável, cujo final parece uma aresta. */
static char *linha_longa(const char *prefixo)
{
    size_t tam = 3000;
    char *s = malloc(tam + 64);
    char *p = s;
    p += sprintf(p, "1 2\n%s", prefixo);
    memset(p, ' ', tam);
    p += tam;
    strcpy(p, "8 9\n6 7\n");
    return s;
}

static void comentario_longo_nao_vira_aresta(void)
{
    char *conteudo = linha_longa("#");
    EdgeList el;
    CHECK(edgelist_ler(fixture(conteudo), &el) == EDGELIST_OK);
    CHECK(el.n == 2);
    CHECK(!tem_aresta(&el, 8, 9));
    CHECK(el.malformadas == 0);
    edgelist_liberar(&el);
    free(conteudo);
}

static void linha_de_dados_longa_e_malformada(void)
{
    char *conteudo = linha_longa("");
    EdgeList el;
    CHECK(edgelist_ler(fixture(conteudo), &el) == EDGELIST_OK);
    CHECK(el.n == 2);
    CHECK(!tem_aresta(&el, 8, 9));
    CHECK(el.malformadas == 1);
    CHECK(el.primeira_malformada == 2);
    CHECK(tem_aresta(&el, 6, 7));   /* a linha seguinte continua sendo lida */
    edgelist_liberar(&el);
    free(conteudo);
}

static void descarta_lacos_e_duplicatas_mantendo_reciprocas(void)
{
    EdgeList el;
    CHECK(edgelist_ler(fixture("3 1\n1 2\n2 2\n1 2\n2 1\n1 2\n5 5\n"), &el) == EDGELIST_OK);
    CHECK(el.lacos == 2);
    CHECK(el.duplicadas == 2);
    CHECK(el.n == 3);
    /* saída ordenada por (origem, destino); 2->1 sobrevive ao lado de 1->2 */
    CHECK(el.n == 3 && el.arestas[0].origem == 1 && el.arestas[0].destino == 2);
    CHECK(el.n == 3 && el.arestas[1].origem == 2 && el.arestas[1].destino == 1);
    CHECK(el.n == 3 && el.arestas[2].origem == 3 && el.arestas[2].destino == 1);
    edgelist_liberar(&el);
}

/* Milhares de arestas, cada uma repetida, em ordem decrescente: passa da
 * capacidade inicial do vetor e exercita ordenação e compactação de verdade. */
static void muitas_arestas_repetidas(void)
{
    const size_t N = 3000;
    FILE *f = fopen(FIXTURE, "wb");
    for (int rep = 0; rep < 2; rep++)
        for (size_t i = N; i-- > 0;)
            fprintf(f, "%zu %zu\n", i, i + 1);
    fclose(f);

    EdgeList el;
    CHECK(edgelist_ler(FIXTURE, &el) == EDGELIST_OK);
    CHECK(el.n == N);
    CHECK(el.duplicadas == N);
    int em_ordem = el.n == N;
    for (size_t k = 0; em_ordem && k < N; k++)
        em_ordem = el.arestas[k].origem == k && el.arestas[k].destino == k + 1;
    CHECK(em_ordem);
    edgelist_liberar(&el);
}

/* Um usuário que segue vários outros: arestas com a mesma origem não podem
 * ser confundidas com duplicatas. */
static void mesma_origem_destinos_distintos(void)
{
    EdgeList el;
    CHECK(edgelist_ler(fixture("1 3\n1 2\n1 4\n1 2\n"), &el) == EDGELIST_OK);
    CHECK(el.n == 3);
    CHECK(el.duplicadas == 1);
    CHECK(tem_aresta(&el, 1, 2));
    CHECK(tem_aresta(&el, 1, 3));
    CHECK(tem_aresta(&el, 1, 4));
    edgelist_liberar(&el);
}

static void primeira_malformada_e_a_primeira(void)
{
    EdgeList el;
    CHECK(edgelist_ler(fixture("1 2\nx\n3 4\ny\n"), &el) == EDGELIST_OK);
    CHECK(el.malformadas == 2);
    CHECK(el.primeira_malformada == 2);
    edgelist_liberar(&el);
}

static void arquivo_inexistente_e_erro(void)
{
    EdgeList el;
    CHECK(edgelist_ler("bin/nao_existe.txt", &el) == EDGELIST_ERRO_ARQUIVO);
    CHECK(el.n == 0);
    CHECK(el.arestas == NULL);
}

/* Um diretório abre com fopen no Linux, mas a leitura falha: o erro de
 * leitura não pode ser confundido com fim de arquivo. */
static void erro_de_leitura_e_erro(void)
{
    EdgeList el;
    CHECK(edgelist_ler("bin", &el) == EDGELIST_ERRO_ARQUIVO);
    CHECK(el.n == 0);
    CHECK(el.arestas == NULL);
}

int main(void)
{
    erro_de_leitura_e_erro();
    arquivo_inexistente_e_erro();
    mesma_origem_destinos_distintos();
    primeira_malformada_e_a_primeira();
    descarta_lacos_e_duplicatas_mantendo_reciprocas();
    muitas_arestas_repetidas();
    comentario_longo_nao_vira_aresta();
    linha_de_dados_longa_e_malformada();
    le_pares_simples();
    ignora_comentarios_e_linhas_vazias();
    aceita_separadores_variados();
    descarta_linhas_malformadas();
    sem_malformadas_primeira_e_zero();

    remove(FIXTURE);
    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
