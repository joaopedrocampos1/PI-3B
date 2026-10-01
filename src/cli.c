#include "cli.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static const char *NOMES_ALGORITMO[] = {
    [ALGO_NENHUM] = "",
    [ALGO_BFS] = "bfs",
    [ALGO_DFS] = "dfs",
    [ALGO_COMPONENTES] = "componentes",
    [ALGO_CICLOS] = "ciclos",
    [ALGO_BIPARTIDO] = "bipartido",
    [ALGO_ARTICULACAO] = "articulacao",
};
#define NUM_ALGORITMOS (sizeof NOMES_ALGORITMO / sizeof NOMES_ALGORITMO[0])

typedef enum { OP_INPUT, OP_STRUCT, OP_ALGO, OP_SOURCE, OP_OUTPUT, OP_AMOSTRAR, OP_SEMENTE_RNG } Opcao;

static const struct {
    const char *nome;
    Opcao id;
} OPCOES[] = {
    {"input", OP_INPUT},   {"struct", OP_STRUCT},     {"algo", OP_ALGO},
    {"source", OP_SOURCE}, {"output", OP_OUTPUT},     {"amostrar", OP_AMOSTRAR},
    {"semente-rng", OP_SEMENTE_RNG},
};
#define NUM_OPCOES (sizeof OPCOES / sizeof OPCOES[0])

static CliStatus falhar(char *erro, size_t tam, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(erro, tam, fmt, ap);
    va_end(ap);
    return CLI_ERRO;
}

/* Inteiro não negativo, só dígitos, sem estourar unsigned long long
 * (mesma regra do leitor de arestas). */
static int ler_numero(const char *s, unsigned long long *valor)
{
    if (!isdigit((unsigned char)*s))
        return 0;
    char *fim;
    errno = 0;
    *valor = strtoull(s, &fim, 10);
    return errno != ERANGE && *fim == '\0';
}

static CliStatus aplicar(Opcoes *op, Opcao id, const char *valor, char *erro, size_t tam)
{
    switch (id) {
    case OP_INPUT:
        op->entrada = valor;
        break;
    case OP_OUTPUT:
        op->saida = valor;
        break;
    case OP_AMOSTRAR:
        op->pasta_amostras = valor;
        break;
    case OP_STRUCT:
        if (strcmp(valor, "lista") == 0)
            op->estrutura = ESTRUTURA_LISTA;
        else if (strcmp(valor, "matriz") == 0)
            op->estrutura = ESTRUTURA_MATRIZ;
        else
            return falhar(erro, tam, "--struct aceita 'lista' ou 'matriz', não '%s'", valor);
        break;
    case OP_ALGO:
        for (size_t a = ALGO_NENHUM + 1; a < NUM_ALGORITMOS; a++)
            if (strcmp(valor, NOMES_ALGORITMO[a]) == 0) {
                op->algoritmo = (Algoritmo)a;
                return CLI_OK;
            }
        return falhar(erro, tam, "algoritmo desconhecido: '%s'", valor);
    case OP_SOURCE:
        if (!ler_numero(valor, &op->origem))
            return falhar(erro, tam, "--source precisa ser um ID inteiro não negativo, não '%s'", valor);
        op->tem_origem = 1;
        break;
    case OP_SEMENTE_RNG:
        if (!ler_numero(valor, &op->semente_rng))
            return falhar(erro, tam, "--semente-rng precisa ser um inteiro não negativo, não '%s'",
                          valor);
        break;
    }
    return CLI_OK;
}

CliStatus cli_ler(int argc, char **argv, Opcoes *op, char *erro, size_t tam_erro)
{
    memset(op, 0, sizeof *op);
    op->estrutura = ESTRUTURA_LISTA;
    op->algoritmo = ALGO_NENHUM;
    op->semente_rng = CLI_SEMENTE_RNG_PADRAO;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
            return CLI_AJUDA;
        if (strncmp(arg, "--", 2) != 0)
            return falhar(erro, tam_erro, "argumento inesperado: '%s'", arg);

        /* "--nome=valor" ou "--nome valor" */
        const char *nome = arg + 2;
        const char *igual = strchr(nome, '=');
        size_t tam_nome = igual ? (size_t)(igual - nome) : strlen(nome);

        size_t k = 0;
        while (k < NUM_OPCOES &&
               !(strlen(OPCOES[k].nome) == tam_nome && strncmp(OPCOES[k].nome, nome, tam_nome) == 0))
            k++;
        if (k == NUM_OPCOES)
            return falhar(erro, tam_erro, "opção desconhecida: '%.*s'", (int)(tam_nome + 2), arg);

        const char *valor = igual ? igual + 1 : (i + 1 < argc ? argv[++i] : NULL);
        /* "--input --struct lista": --struct é outra opção, não um nome de arquivo */
        if (!valor || *valor == '\0' || (!igual && strncmp(valor, "--", 2) == 0))
            return falhar(erro, tam_erro, "--%s precisa de um valor", OPCOES[k].nome);

        CliStatus st = aplicar(op, OPCOES[k].id, valor, erro, tam_erro);
        if (st != CLI_OK)
            return st;
    }

    if (!op->entrada)
        return falhar(erro, tam_erro, "falta o arquivo de entrada: --input <arquivo>");
    if (op->pasta_amostras && op->algoritmo != ALGO_NENHUM)
        return falhar(erro, tam_erro, "--amostrar não pode ser usado junto com --algo");
    return CLI_OK;
}

void cli_uso(FILE *f, const char *programa)
{
    fprintf(f,
            "uso: %s --input <arquivo> [opções]\n"
            "\n"
            "  --input <arquivo>      edge list (uma aresta \"origem destino\" por linha)\n"
            "  --struct lista|matriz  representação do grafo (padrão: lista)\n"
            "  --algo <nome>          bfs, dfs, componentes, ciclos, bipartido, articulacao\n"
            "  --source <id>          vértice de origem, para os algoritmos que usam um\n"
            "  --output <arquivo>     arquivo de saída dos resultados\n"
            "  --amostrar <pasta>     grava as amostras por BFS (3 sementes x N = 100, 250,\n"
            "                         500 e 1.000) em <pasta>\n"
            "  --semente-rng <n>      semente do sorteio das amostras (padrão: %llu)\n"
            "  --help                 mostra esta ajuda\n"
            "\n"
            "Toda opção aceita também a forma --opcao=valor.\n"
            "Sem --algo nem --amostrar, só lê o arquivo e mostra um resumo.\n"
            "\n"
            "exemplos:\n"
            "  %s --input data/raw/twitter_combined.txt\n"
            "  %s --input data/raw/twitter_combined.txt --amostrar data/samples\n",
            programa, CLI_SEMENTE_RNG_PADRAO, programa, programa);
}

const char *cli_nome_algoritmo(Algoritmo a)
{
    return (size_t)a < NUM_ALGORITMOS ? NOMES_ALGORITMO[a] : "";
}
