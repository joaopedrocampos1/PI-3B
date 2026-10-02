/*
 * Testes do logger em CSV (src/logger.c).
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_logger && ./bin/test_logger
 */
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARQUIVO "bin/fixture_log.csv"

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

/* Linha `k` (0 = primeira) do arquivo, sem o '\n', em `buf`. */
static int linha(int k, char *buf, size_t tam)
{
    FILE *f = fopen(ARQUIVO, "rb");
    if (!f)
        return 0;
    int ok = 0;
    for (int i = 0; i <= k; i++)
        if (!(ok = fgets(buf, (int)tam, f) != NULL))
            break;
    fclose(f);
    if (ok)
        buf[strcspn(buf, "\n")] = '\0';
    return ok;
}

/* O timestamp tem forma fixa: AAAA-MM-DDTHH:MM:SSZ seguido de vírgula. */
static int timestamp_valido(const char *s)
{
    const char *forma = "dddd-dd-ddTdd:dd:ddZ,";
    for (; *forma; forma++, s++)
        if (*forma == 'd' ? (*s < '0' || *s > '9') : *s != *forma)
            return 0;
    return 1;
}

static void cria_com_cabecalho_e_acrescenta(void)
{
    remove(ARQUIVO);
    LogRegistro r = {"data/samples/bfs_1_n100.txt", 100, 342, "lista", "bfs",
                     1.23456, 12.75, 1};
    CHECK(log_registrar(ARQUIVO, &r) == LOG_OK);
    r.execucao = 2;
    r.estrutura = "matriz";
    CHECK(log_registrar(ARQUIVO, &r) == LOG_OK);

    char buf[512];
    CHECK(linha(0, buf, sizeof buf));
    CHECK(strcmp(buf, "timestamp,dataset,N,M,estrutura,algoritmo,tempo_ms,memoria_kb,"
                      "execucao_num") == 0);

    CHECK(linha(1, buf, sizeof buf));
    CHECK(timestamp_valido(buf));
    CHECK(strcmp(buf + 21, "data/samples/bfs_1_n100.txt,100,342,lista,bfs,1.235,12.8,1") == 0);

    CHECK(linha(2, buf, sizeof buf));
    CHECK(strcmp(buf + 21, "data/samples/bfs_1_n100.txt,100,342,matriz,bfs,1.235,12.8,2") == 0);

    CHECK(!linha(3, buf, sizeof buf));   /* cabeçalho não se repete */
}

static void escapa_campos(void)
{
    remove(ARQUIVO);
    LogRegistro r = {"pasta, com \"aspas\"/g.txt", 3, 2, "lista", NULL, 0, 0, 7};
    CHECK(log_registrar(ARQUIVO, &r) == LOG_OK);

    char buf[512];
    CHECK(linha(1, buf, sizeof buf));
    CHECK(strcmp(buf + 21, "\"pasta, com \"\"aspas\"\"/g.txt\",3,2,lista,,0.000,0.0,7") == 0);
}

static void pasta_inexistente(void)
{
    LogRegistro r = {"x", 1, 0, "lista", "bfs", 0, 0, 1};
    CHECK(log_registrar("bin/nao_existe/log.csv", &r) == LOG_ERRO_ARQUIVO);
}

int main(void)
{
    cria_com_cabecalho_e_acrescenta();
    escapa_campos();
    pasta_inexistente();

    remove(ARQUIVO);
    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
