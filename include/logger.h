#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>

/*
 * Logger em CSV (#25, RF03): uma linha por execução de algoritmo, matéria-
 * -prima dos gráficos (#28) e da análise comparativa (#29).
 *
 * Colunas, nesta ordem:
 *   timestamp, dataset, N, M, estrutura, algoritmo, tempo_ms, memoria_kb,
 *   execucao_num
 *
 * - timestamp em UTC, ISO 8601 ("2026-10-01T22:15:03Z").
 * - O arquivo é sempre aberto para acrescentar: nada já gravado se perde.
 *   O cabeçalho só é escrito quando o arquivo está vazio ou não existe.
 * - Campos de texto com vírgula, aspas ou quebra de linha saem entre aspas,
 *   como manda o CSV, então caminhos de dataset quaisquer não quebram colunas.
 */

#define LOG_CAMINHO_PADRAO "results/log.csv"

typedef struct {
    const char *dataset;     /* caminho do arquivo de entrada */
    size_t n;                /* vértices */
    size_t m;                /* arestas */
    const char *estrutura;   /* "lista" ou "matriz" */
    const char *algoritmo;   /* "bfs", "dfs", ... */
    double tempo_ms;
    double memoria_kb;
    unsigned execucao;       /* repetição: 1 é o aquecimento, descartado na análise */
} LogRegistro;

typedef enum {
    LOG_OK = 0,
    LOG_ERRO_ARQUIVO   /* não abriu ou não gravou (a pasta existe?) */
} LogStatus;

/* Acrescenta `r` ao CSV em `caminho`, criando o arquivo com cabeçalho se
 * preciso. */
LogStatus log_registrar(const char *caminho, const LogRegistro *r);

#endif
