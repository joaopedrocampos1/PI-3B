/*
 * Testes do mapeamento de IDs (src/idmap.c).
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   gcc -std=c11 -Wall -Wextra -Iinclude src/idmap.c src/edgelist.c tests/test_idmap.c -o bin/test_idmap
 *   ./bin/test_idmap
 *
 * O teste com dados reais usa o recorte versionado de 10 mil vértices
 * (data/recorte_n10000.txt, #59): roda sem baixar nada.
 */
#include "edgelist.h"
#include "memtrack.h"
#include "idmap.h"

#include <limits.h>
#include <stdio.h>

#define DATASET  "data/recorte_n10000.txt"
#define MAIOR_ID 567262610ULL   /* maior ID do recorte */

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

static void indices_seguem_ordem_de_chegada(void)
{
    IdMap m;
    idmap_iniciar(&m);
    size_t i;

    CHECK(idmap_inserir(&m, 214328887ULL, &i) && i == 0);
    CHECK(idmap_inserir(&m, 17116707ULL, &i) && i == 1);
    CHECK(idmap_inserir(&m, 214328887ULL, &i) && i == 0);   /* repetido */
    CHECK(idmap_inserir(&m, 380580781ULL, &i) && i == 2);
    CHECK(m.n == 3);

    CHECK(idmap_original(&m, 0) == 214328887ULL);
    CHECK(idmap_original(&m, 1) == 17116707ULL);
    CHECK(idmap_original(&m, 2) == 380580781ULL);
    idmap_liberar(&m);
}

/* Muito mais IDs que a capacidade inicial. Múltiplos de 1024 colidiriam
 * todos no mesmo slot se o hash fosse o próprio ID. */
static void cresce_alem_da_capacidade_inicial(void)
{
    const size_t N = 100000;
    IdMap m;
    idmap_iniciar(&m);

    int ok = 1;
    for (size_t k = 0; k < N; k++) {
        size_t i;
        ok = ok && idmap_inserir(&m, (unsigned long long)k * 1024 + 7, &i) && i == k;
    }
    CHECK(ok);
    CHECK(m.n == N);

    /* depois de todas as realocações, tudo continua no lugar */
    int ida_e_volta = 1;
    for (size_t k = 0; k < N; k++) {
        size_t i;
        unsigned long long id = (unsigned long long)k * 1024 + 7;
        ida_e_volta = ida_e_volta && idmap_inserir(&m, id, &i) && i == k &&
                      idmap_original(&m, k) == id;
    }
    CHECK(ida_e_volta);
    CHECK(m.n == N);
    idmap_liberar(&m);
}

static void buscar_nao_insere(void)
{
    IdMap m;
    idmap_iniciar(&m);
    size_t i = 99;

    CHECK(!idmap_buscar(&m, 42, &i));          /* mapa vazio, tabela nem alocada */

    idmap_inserir(&m, 42, &i);
    idmap_inserir(&m, 7, &i);

    CHECK(idmap_buscar(&m, 7, &i) && i == 1);
    CHECK(idmap_buscar(&m, 42, &i) && i == 0);
    CHECK(!idmap_buscar(&m, 1000, &i));
    CHECK(m.n == 2);                            /* a busca falha não criou nada */
    idmap_liberar(&m);
}

/* O ID 0 não pode ser confundido com "slot vazio", e o maior valor
 * representável também é um ID válido. */
static void ids_nos_extremos(void)
{
    IdMap m;
    idmap_iniciar(&m);
    size_t i;

    CHECK(idmap_inserir(&m, 0, &i) && i == 0);
    CHECK(idmap_inserir(&m, ULLONG_MAX, &i) && i == 1);
    CHECK(idmap_inserir(&m, 0, &i) && i == 0);
    CHECK(idmap_buscar(&m, 0, &i) && i == 0);
    CHECK(idmap_buscar(&m, ULLONG_MAX, &i) && i == 1);
    CHECK(idmap_original(&m, 0) == 0);
    CHECK(idmap_original(&m, 1) == ULLONG_MAX);
    CHECK(m.n == 2);
    idmap_liberar(&m);
}

/* Critério de aceite da issue #7: mapear todos os usuários de dados reais.
 * Desde a #59, a instância real do protocolo é o recorte de 10 mil vértices;
 * os valores esperados vêm da contagem independente feita ao gerá-lo. O
 * arquivo é versionado, então a falta dele é falha, e não teste pulado. */
static void dataset_recorte(void)
{
    EdgeList el;
    if (edgelist_ler(DATASET, &el) != EDGELIST_OK) {
        verificacoes++;
        falhas++;
        fprintf(stderr, "  FALHOU: não foi possível ler %s (rode a partir da raiz do repositório)\n",
                DATASET);
        return;
    }
    CHECK(el.n == 376035);

    IdMap m;
    idmap_iniciar(&m);
    int ok = 1;
    for (size_t e = 0; e < el.n; e++) {
        size_t i;
        ok = ok && idmap_inserir(&m, el.arestas[e].origem, &i)
                && idmap_inserir(&m, el.arestas[e].destino, &i);
    }
    CHECK(ok);
    CHECK(m.n == 10000);

    size_t i;
    CHECK(idmap_buscar(&m, 12, &i) && i == 0);   /* primeira aresta: 12 -> 13 */
    CHECK(idmap_buscar(&m, 13, &i) && i == 1);
    CHECK(idmap_buscar(&m, MAIOR_ID, &i));        /* maior ID do arquivo */

    /* nenhum índice compartilhado: cada um volta para o próprio ID */
    int ida_e_volta = 1;
    for (size_t k = 0; k < m.n; k++)
        ida_e_volta = ida_e_volta && idmap_buscar(&m, idmap_original(&m, k), &i) && i == k;
    CHECK(ida_e_volta);

    idmap_liberar(&m);
    edgelist_liberar(&el);
}

/* A tabela e o mapa inverso passam pelo memtrack (RF03): aparecem na
 * medição enquanto existem e somem dela quando o mapa é liberado. */
static void memoria_medida_pelo_memtrack(void)
{
    size_t antes = mt_current_bytes();
    IdMap m;
    idmap_iniciar(&m);
    size_t i;
    for (unsigned long long id = 0; id < 1000; id++)
        idmap_inserir(&m, id * 7919, &i);
    CHECK(mt_current_bytes() > antes);
    idmap_liberar(&m);
    CHECK(mt_current_bytes() == antes);
}

int main(void)
{
    memoria_medida_pelo_memtrack();
    dataset_recorte();
    ids_nos_extremos();
    buscar_nao_insere();
    indices_seguem_ordem_de_chegada();
    cresce_alem_da_capacidade_inicial();

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
