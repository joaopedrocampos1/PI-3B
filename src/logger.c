#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define CABECALHO \
    "timestamp,dataset,N,M,estrutura,algoritmo,tempo_ms,memoria_kb,execucao_num\n"

/* Escreve `s` como campo CSV: entre aspas, com aspas dobradas, só se
 * precisar. */
static void campo(FILE *f, const char *s)
{
    if (!s)
        s = "";
    if (!strpbrk(s, ",\"\r\n")) {
        fputs(s, f);
        return;
    }
    fputc('"', f);
    for (; *s; s++) {
        if (*s == '"')
            fputc('"', f);
        fputc(*s, f);
    }
    fputc('"', f);
}

LogStatus log_registrar(const char *caminho, const LogRegistro *r)
{
    FILE *f = fopen(caminho, "ab");
    if (!f)
        return LOG_ERRO_ARQUIVO;

    /* em modo de acréscimo, a posição inicial não é garantida pelo padrão */
    if (fseek(f, 0, SEEK_END) == 0 && ftell(f) == 0)
        fputs(CABECALHO, f);

    char quando[32];
    time_t agora = time(NULL);
    strftime(quando, sizeof quando, "%Y-%m-%dT%H:%M:%SZ", gmtime(&agora));

    fprintf(f, "%s,", quando);
    campo(f, r->dataset);
    fprintf(f, ",%zu,%zu,", r->n, r->m);
    campo(f, r->estrutura);
    fputc(',', f);
    campo(f, r->algoritmo);
    fprintf(f, ",%.3f,%.1f,%u\n", r->tempo_ms, r->memoria_kb, r->execucao);

    int erro = ferror(f);
    if (fclose(f) != 0 || erro)
        return LOG_ERRO_ARQUIVO;
    return LOG_OK;
}
