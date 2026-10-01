#include "cli.h"
#include "edgelist.h"
#include "idmap.h"
#include "subgraph.h"

#include <stdio.h>

/* Protocolo experimental: 3 sementes x 4 tamanhos de amostra. */
static const size_t TAMANHOS[] = {100, 250, 500, 1000};
#define NUM_TAMANHOS (sizeof TAMANHOS / sizeof TAMANHOS[0])
#define NUM_SEMENTES 3

/* Número de vértices distintos entre as arestas lidas. */
static int contar_vertices(const EdgeList *el, size_t *v)
{
    IdMap m;
    idmap_iniciar(&m);
    size_t i;
    for (size_t e = 0; e < el->n; e++)
        if (!idmap_inserir(&m, el->arestas[e].origem, &i) ||
            !idmap_inserir(&m, el->arestas[e].destino, &i)) {
            idmap_liberar(&m);
            return 0;
        }
    *v = m.n;
    idmap_liberar(&m);
    return 1;
}

static void resumir(const char *caminho, const EdgeList *el, size_t v)
{
    printf("%s: %zu vértices, %zu arestas\n", caminho, v, el->n);
    if (el->duplicadas || el->lacos)
        printf("  descartadas: %zu duplicadas, %zu laços\n", el->duplicadas, el->lacos);
    if (el->malformadas)
        printf("  ignoradas: %zu linhas malformadas (a primeira na linha %zu)\n",
               el->malformadas, el->primeira_malformada);
}

/* Grava em op->pasta_amostras um arquivo por semente e tamanho. Devolve o
 * código de saída do programa. */
static int gerar_amostras(const EdgeList *el, const Opcoes *op)
{
    Amostrador a;
    if (subgraph_preparar(&a, el) != AMOSTRA_OK) {
        fprintf(stderr, "erro: memória insuficiente para preparar a amostragem\n");
        return 1;
    }

    unsigned long long sementes[NUM_SEMENTES];
    AmostraStatus st = subgraph_sortear_sementes(&a, op->semente_rng, NUM_SEMENTES, sementes);
    if (st != AMOSTRA_OK) {
        fprintf(stderr, "erro: %s\n", st == AMOSTRA_SEMENTES_INSUFICIENTES
                                          ? "o grafo tem menos vértices que o número de sementes"
                                          : "memória insuficiente para sortear as sementes");
        subgraph_liberar(&a);
        return 1;
    }

    int codigo = 0;
    for (size_t s = 0; s < NUM_SEMENTES && codigo != 1; s++) {
        for (size_t t = 0; t < NUM_TAMANHOS; t++) {
            size_t n = TAMANHOS[t];
            EdgeList sub;
            st = subgraph_bfs(&a, sementes[s], n, &sub);
            if (st == AMOSTRA_COMPONENTE_PEQUENO) {
                fprintf(stderr, "aviso: a semente %llu alcança menos de %zu vértices; amostra pulada\n",
                        sementes[s], n);
                codigo = 2;
                continue;
            }
            if (st != AMOSTRA_OK) {
                fprintf(stderr, "erro: memória insuficiente para gerar a amostra\n");
                codigo = 1;
                break;
            }

            char caminho[1024], comentario[1024];
            int c1 = snprintf(caminho, sizeof caminho, "%s/bfs_%llu_n%zu.txt",
                              op->pasta_amostras, sementes[s], n);
            snprintf(comentario, sizeof comentario,
                     "subgrafo induzido pelos %zu primeiros vértices de um BFS sem sentido\n"
                     "dataset: %s\nsemente-rng: %llu\nsemente: %llu\nN: %zu\narestas: %zu",
                     n, op->entrada, op->semente_rng, sementes[s], n, sub.n);

            if (c1 < 0 || (size_t)c1 >= sizeof caminho ||
                edgelist_gravar(caminho, &sub, comentario) != EDGELIST_OK) {
                fprintf(stderr, "erro: não foi possível gravar '%s' (a pasta existe?)\n", caminho);
                edgelist_liberar(&sub);
                codigo = 1;
                break;
            }
            printf("%s: %zu vértices, %zu arestas\n", caminho, n, sub.n);
            edgelist_liberar(&sub);
        }
    }

    subgraph_liberar(&a);
    return codigo;
}

int main(int argc, char **argv)
{
    Opcoes op;
    char erro[256];
    CliStatus st = cli_ler(argc, argv, &op, erro, sizeof erro);
    if (st == CLI_AJUDA) {
        cli_uso(stdout, argv[0]);
        return 0;
    }
    if (st == CLI_ERRO) {
        fprintf(stderr, "erro: %s\n\n", erro);
        cli_uso(stderr, argv[0]);
        return 1;
    }

    EdgeList el;
    EdgeListStatus le = edgelist_ler(op.entrada, &el);
    if (le != EDGELIST_OK) {
        fprintf(stderr, "erro: %s '%s'\n",
                le == EDGELIST_ERRO_MEMORIA ? "memória insuficiente para ler"
                                            : "não foi possível ler",
                op.entrada);
        return 1;
    }

    size_t v;
    if (!contar_vertices(&el, &v)) {
        fprintf(stderr, "erro: memória insuficiente para mapear os vértices\n");
        edgelist_liberar(&el);
        return 1;
    }
    resumir(op.entrada, &el, v);

    int codigo = 0;
    if (op.pasta_amostras) {
        codigo = gerar_amostras(&el, &op);
    } else if (op.algoritmo != ALGO_NENHUM) {
        fprintf(stderr, "o algoritmo '%s' ainda não foi implementado\n",
                cli_nome_algoritmo(op.algoritmo));
        codigo = 2;
    }

    edgelist_liberar(&el);
    return codigo;
}
