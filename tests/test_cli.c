/*
 * Testes da leitura de argumentos (src/cli.c).
 *
 * Rodar com: make test
 */
#include "cli.h"

#include <stdarg.h>
#include <stdio.h>
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

static char erro[256];

/* Chama cli_ler com "grafos" seguido dos argumentos (lista terminada em NULL). */
static CliStatus ler(Opcoes *op, ...)
{
    static char *argv[32];
    int argc = 0;
    argv[argc++] = "grafos";

    va_list ap;
    va_start(ap, op);
    char *arg;
    while ((arg = va_arg(ap, char *)) != NULL)
        argv[argc++] = arg;
    va_end(ap);

    erro[0] = '\0';
    return cli_ler(argc, argv, op, erro, sizeof erro);
}

static void so_entrada_usa_padroes(void)
{
    Opcoes op;
    CHECK(ler(&op, "--input", "dados.txt", NULL) == CLI_OK);
    CHECK(op.entrada && strcmp(op.entrada, "dados.txt") == 0);
    CHECK(op.estrutura == ESTRUTURA_LISTA);
    CHECK(op.algoritmo == ALGO_NENHUM);
    CHECK(!op.tem_origem);
    CHECK(op.saida == NULL);
    CHECK(op.pasta_amostras == NULL);
    CHECK(op.semente_rng == 2026ULL);
    CHECK(op.execucao == 1);
    CHECK(op.log && strcmp(op.log, "results/log.csv") == 0);
}

static void execucao_e_log(void)
{
    Opcoes op;
    CHECK(ler(&op, "--input", "d.txt", "--execucao", "11", "--log=bin/x.csv", NULL) == CLI_OK);
    CHECK(op.execucao == 11);
    CHECK(op.log && strcmp(op.log, "bin/x.csv") == 0);
}

static void todas_as_opcoes_separadas(void)
{
    Opcoes op;
    CHECK(ler(&op, "--input", "d.txt", "--struct", "matriz", "--algo", "articulacao",
              "--source", "568770231", "--output", "r.txt", NULL) == CLI_OK);
    CHECK(op.estrutura == ESTRUTURA_MATRIZ);
    CHECK(op.algoritmo == ALGO_ARTICULACAO);
    CHECK(op.tem_origem && op.origem == 568770231ULL);
    CHECK(op.saida && strcmp(op.saida, "r.txt") == 0);
}

static void todas_as_opcoes_com_igual(void)
{
    Opcoes op;
    CHECK(ler(&op, "--input=d.txt", "--struct=matriz", "--algo=bfs", "--source=0",
              "--output=r.txt", NULL) == CLI_OK);
    CHECK(op.entrada && strcmp(op.entrada, "d.txt") == 0);
    CHECK(op.estrutura == ESTRUTURA_MATRIZ);
    CHECK(op.algoritmo == ALGO_BFS);
    CHECK(op.tem_origem && op.origem == 0);
    CHECK(op.saida && strcmp(op.saida, "r.txt") == 0);
}

static void modo_amostragem(void)
{
    Opcoes op;
    CHECK(ler(&op, "--input", "d.txt", "--amostrar", "data/samples", "--semente-rng", "7",
              NULL) == CLI_OK);
    CHECK(op.pasta_amostras && strcmp(op.pasta_amostras, "data/samples") == 0);
    CHECK(op.semente_rng == 7);
}

/* Todos os nomes de algoritmo aceitos, ida e volta. */
static void nomes_de_algoritmo(void)
{
    static const struct { const char *nome; Algoritmo a; } casos[] = {
        {"bfs", ALGO_BFS},           {"dfs", ALGO_DFS},
        {"componentes", ALGO_COMPONENTES}, {"ciclos", ALGO_CICLOS},
        {"bipartido", ALGO_BIPARTIDO}, {"articulacao", ALGO_ARTICULACAO},
    };
    for (size_t i = 0; i < sizeof casos / sizeof casos[0]; i++) {
        Opcoes op;
        CHECK(ler(&op, "--input", "d.txt", "--algo", casos[i].nome, NULL) == CLI_OK);
        CHECK(op.algoritmo == casos[i].a);
        CHECK(strcmp(cli_nome_algoritmo(casos[i].a), casos[i].nome) == 0);
    }
}

static void repetida_vale_a_ultima(void)
{
    Opcoes op;
    CHECK(ler(&op, "--input", "a.txt", "--struct", "matriz", "--input", "b.txt",
              "--struct", "lista", NULL) == CLI_OK);
    CHECK(op.entrada && strcmp(op.entrada, "b.txt") == 0);
    CHECK(op.estrutura == ESTRUTURA_LISTA);
}

/* Cada caso: argumentos (até 6) e um trecho que a mensagem de erro precisa
 * conter, para o usuário saber o que corrigir. */
static void argumentos_invalidos(void)
{
    static const struct {
        const char *args[7];
        const char *trecho;
    } casos[] = {
        {{NULL}, "--input"},                                             /* nada */
        {{"--struct", "lista", NULL}, "--input"},                         /* sem --input */
        {{"--input", NULL}, "--input"},                                   /* falta o valor */
        {{"--input", "--struct", "lista", NULL}, "--input"},              /* valor é outra opção */
        {{"--input=", NULL}, "--input"},                                  /* valor vazio */
        {{"--input", "d", "--struct", "arvore", NULL}, "arvore"},
        {{"--input", "d", "--algo", "dijkstra", NULL}, "dijkstra"},
        {{"--input", "d", "--source", "abc", NULL}, "abc"},
        {{"--input", "d", "--source", "-1", NULL}, "-1"},
        {{"--input", "d", "--source", "+1", NULL}, "+1"},
        {{"--input", "d", "--source", "99999999999999999999999", NULL}, "--source"},
        {{"--input", "d", "--semente-rng", "x", NULL}, "--semente-rng"},
        {{"--input", "d", "--execucao", "0", NULL}, "--execucao"},
        {{"--input", "d", "--execucao", "4294967296", NULL}, "--execucao"},  /* > UINT_MAX */
        {{"--input", "d", "--log", NULL}, "--log"},
        {{"--input", "d", "--verbose", NULL}, "--verbose"},
        {{"--inputx=d", NULL}, "--inputx"},                              /* nome mais longo */
        {{"--inp", "d", NULL}, "--inp"},                                  /* abreviação */
        {{"--input", "d", "--source", "12abc", NULL}, "12abc"},           /* lixo após o número */
        {{"--input", "d", "extra", NULL}, "extra"},
        {{"-input", "d", NULL}, "-input"},
        {{"--input", "d", "--amostrar", "p", "--algo", "bfs", NULL}, "--amostrar"},
    };

    for (size_t i = 0; i < sizeof casos / sizeof casos[0]; i++) {
        char *argv[8] = {"grafos"};
        int argc = 1;
        for (int j = 0; casos[i].args[j]; j++)
            argv[argc++] = (char *)casos[i].args[j];

        Opcoes op;
        erro[0] = '\0';
        CliStatus st = cli_ler(argc, argv, &op, erro, sizeof erro);
        int ok = st == CLI_ERRO && strstr(erro, casos[i].trecho) != NULL;
        if (!ok)
            fprintf(stderr, "  caso %zu (%s ...): status=%d erro=\"%s\"\n", i,
                    argc > 1 ? argv[1] : "(vazio)", st, erro);
        CHECK(ok);
    }
}

static void ajuda(void)
{
    Opcoes op;
    CHECK(ler(&op, "--help", NULL) == CLI_AJUDA);
    CHECK(ler(&op, "-h", NULL) == CLI_AJUDA);
    CHECK(ler(&op, "--input", "d.txt", "--help", NULL) == CLI_AJUDA);
}

int main(void)
{
    argumentos_invalidos();
    ajuda();
    so_entrada_usa_padroes();
    execucao_e_log();
    todas_as_opcoes_separadas();
    todas_as_opcoes_com_igual();
    modo_amostragem();
    nomes_de_algoritmo();
    repetida_vale_a_ultima();

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
