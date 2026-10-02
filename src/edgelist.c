#include "edgelist.h"
#include "memtrack.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_LINHA 1024

/* Acrescenta uma aresta ao vetor, dobrando a capacidade quando necessário. */
static int anexar(EdgeList *el, size_t *cap, unsigned long long o, unsigned long long d)
{
    if (el->n == *cap) {
        size_t nova = *cap ? *cap * 2 : 1024;
        Aresta *p = mt_realloc(el->arestas, nova * sizeof *p);
        if (!p)
            return 0;
        el->arestas = p;
        *cap = nova;
    }
    el->arestas[el->n].origem = o;
    el->arestas[el->n].destino = d;
    el->n++;
    return 1;
}

static int eh_separador(char c)
{
    return c == ' ' || c == '\t' || c == ',' || c == ';' || c == '\r' || c == '\n';
}

static char *pular_separadores(char *p)
{
    while (*p && eh_separador(*p))
        p++;
    return p;
}

/* Lê um inteiro não negativo a partir de *p e avança o cursor.
 * Devolve 0 se o campo não for só dígitos ou não couber em unsigned long long. */
static int ler_campo(char **p, unsigned long long *valor)
{
    char *fim;
    *p = pular_separadores(*p);

    /* strtoull aceitaria sinal e espaços: só dígitos passam daqui */
    if (!isdigit((unsigned char)**p))
        return 0;

    errno = 0;
    *valor = strtoull(*p, &fim, 10);
    if (errno == ERANGE)
        return 0;

    /* lixo colado ao número ("4x") não precisa de checagem aqui: o cursor
     * para no 'x', que reprova o campo seguinte ou o fim da linha */
    *p = fim;
    return 1;
}

/* Se `linha` não coube no buffer do fgets, consome o restante dela no arquivo
 * e devolve 1. Sem isso, a continuação seria lida como uma linha nova. */
static int descartar_se_truncada(FILE *f, const char *linha)
{
    size_t len = strlen(linha);
    if (len == 0 || linha[len - 1] == '\n')
        return 0;

    int c = fgetc(f);
    if (c == EOF || c == '\n')   /* última linha sem '\n', ou coube exatamente */
        return 0;

    while ((c = fgetc(f)) != EOF && c != '\n')
        ;
    return 1;
}

static int comparar_arestas(const void *a, const void *b)
{
    const Aresta *x = a, *y = b;
    /* comparação explícita: subtrair unsigned long long daria overflow */
    if (x->origem != y->origem)
        return x->origem < y->origem ? -1 : 1;
    if (x->destino != y->destino)
        return x->destino < y->destino ? -1 : 1;
    return 0;
}

/* Ordena as arestas e remove as repetidas, contando quantas saíram. */
static void remover_duplicatas(EdgeList *el)
{
    if (el->n == 0)
        return;

    qsort(el->arestas, el->n, sizeof *el->arestas, comparar_arestas);

    size_t escrita = 1;
    for (size_t i = 1; i < el->n; i++)
        if (comparar_arestas(&el->arestas[i], &el->arestas[escrita - 1]) != 0)
            el->arestas[escrita++] = el->arestas[i];

    el->duplicadas = el->n - escrita;
    el->n = escrita;
}

EdgeListStatus edgelist_ler(const char *caminho, EdgeList *saida)
{
    memset(saida, 0, sizeof *saida);

    FILE *f = fopen(caminho, "r");
    if (!f)
        return EDGELIST_ERRO_ARQUIVO;

    size_t cap = 0;
    size_t num_linha = 0;
    char linha[TAM_LINHA];

    while (fgets(linha, sizeof linha, f)) {
        num_linha++;
        int truncada = descartar_se_truncada(f, linha);

        char *p = pular_separadores(linha);
        if (*p == '#')
            continue;
        if (*p == '\0' && !truncada)
            continue;

        unsigned long long o, d;
        if (truncada || !ler_campo(&p, &o) || !ler_campo(&p, &d) ||
            *pular_separadores(p) != '\0') {
            if (saida->malformadas++ == 0)
                saida->primeira_malformada = num_linha;
            continue;
        }

        if (o == d) {
            saida->lacos++;
            continue;
        }

        if (!anexar(saida, &cap, o, d)) {
            fclose(f);
            edgelist_liberar(saida);
            return EDGELIST_ERRO_MEMORIA;
        }
    }

    /* fgets devolve NULL tanto no fim do arquivo quanto em erro de leitura */
    int erro_leitura = ferror(f);
    fclose(f);
    if (erro_leitura) {
        edgelist_liberar(saida);
        return EDGELIST_ERRO_ARQUIVO;
    }

    remover_duplicatas(saida);
    return EDGELIST_OK;
}

EdgeListStatus edgelist_gravar(const char *caminho, const EdgeList *el, const char *comentario)
{
    FILE *f = fopen(caminho, "w");
    if (!f)
        return EDGELIST_ERRO_ARQUIVO;

    /* cada linha do comentário começa com "# ", senão viraria dado na leitura */
    if (comentario) {
        fputs("# ", f);
        for (const char *c = comentario; *c; c++) {
            fputc(*c, f);
            if (*c == '\n' && c[1] != '\0')
                fputs("# ", f);
        }
        if (comentario[0] == '\0' || comentario[strlen(comentario) - 1] != '\n')
            fputc('\n', f);
    }

    for (size_t i = 0; i < el->n; i++)
        fprintf(f, "%llu %llu\n", el->arestas[i].origem, el->arestas[i].destino);

    /* erros de escrita (disco cheio, etc.) só aparecem em ferror ou no fclose */
    int erro = ferror(f);
    if (fclose(f) != 0 || erro)
        return EDGELIST_ERRO_ARQUIVO;
    return EDGELIST_OK;
}

void edgelist_liberar(EdgeList *el)
{
    mt_free(el->arestas);
    memset(el, 0, sizeof *el);
}
