#include "separacao.h"
#include "bfs.h"
#include "memtrack.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void separacao_grau_entrada(const Graph *g, size_t *grau)
{
    size_t n = graph_num_vertices(g);
    memset(grau, 0, n * sizeof *grau);
    for (size_t u = 0; u < n; u++) {
        GraphIter it;
        size_t w;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next(&it, &w))
            grau[w]++;
    }
}

void separacao_liberar(SeparacaoResultado *r)
{
    mt_free(r->eh_influenciador);
    mt_free(r->perfis);
    mt_free(r->hist_rede);
    mt_free(r->hist_influenciador);
    memset(r, 0, sizeof *r);
}

static int decrescente(const void *a, const void *b)
{
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x < y) - (x > y);
}

static int crescente(const void *a, const void *b)
{
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x > y) - (x < y);
}

/* Marca os influenciadores em r->eh_influenciador: os ceil(fracao * V) de
 * maior grau de entrada, mais os empatados com o último. */
static SeparacaoStatus marcar_influenciadores(const Graph *g, double fracao, SeparacaoResultado *r)
{
    size_t n = graph_num_vertices(g);
    size_t *grau = mt_malloc(n * sizeof *grau);
    size_t *ordenado = mt_malloc(n * sizeof *ordenado);
    r->eh_influenciador = mt_calloc(n, 1);
    if (!grau || !ordenado || !r->eh_influenciador) {
        mt_free(grau);
        mt_free(ordenado);
        return SEPARACAO_ERRO_MEMORIA;
    }

    separacao_grau_entrada(g, grau);
    memcpy(ordenado, grau, n * sizeof *grau);
    qsort(ordenado, n, sizeof *ordenado, decrescente);

    size_t k = (size_t)ceil(fracao * (double)n);
    if (k < 1)
        k = 1;
    if (k > n)
        k = n;
    r->grau_entrada_corte = ordenado[k - 1];

    for (size_t v = 0; v < n; v++)
        if (grau[v] >= r->grau_entrada_corte) {
            r->eh_influenciador[v] = 1;
            r->num_influenciadores++;
        }
    mt_free(grau);
    mt_free(ordenado);
    return SEPARACAO_OK;
}

/* splitmix64: gerador pequeno e reprodutível em qualquer plataforma, ao
 * contrário de rand(). */
static uint64_t proximo_aleatorio(uint64_t *estado)
{
    uint64_t z = (*estado += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Sorteia até `quantos` vértices não influenciadores, sem reposição
 * (Fisher-Yates parcial), e devolve em `sorteados`, em ordem crescente.
 * Devolve quantos foram sorteados, ou SIZE_MAX se faltar memória. */
static size_t sortear_perfis(const SeparacaoResultado *r, size_t n, size_t quantos,
                             unsigned long long semente, size_t *sorteados)
{
    size_t *cand = mt_malloc((n ? n : 1) * sizeof *cand);
    if (!cand)
        return SIZE_MAX;
    size_t c = 0;
    for (size_t v = 0; v < n; v++)
        if (!r->eh_influenciador[v])
            cand[c++] = v;

    if (quantos > c)
        quantos = c;
    uint64_t estado = semente;
    for (size_t i = 0; i < quantos; i++) {
        size_t j = i + (size_t)(proximo_aleatorio(&estado) % (c - i));
        size_t t = cand[i];
        cand[i] = cand[j];
        cand[j] = t;
        sorteados[i] = cand[i];
    }
    mt_free(cand);
    qsort(sorteados, quantos, sizeof *sorteados, crescente);
    return quantos;
}

/* Garante que os dois histogramas tenham pelo menos `tam` posições. */
static int garantir_hist(SeparacaoResultado *r, size_t tam)
{
    if (tam <= r->hist_tam)
        return 1;
    size_t novo = r->hist_tam ? r->hist_tam : 16;
    while (novo < tam)
        novo *= 2;
    size_t *a = mt_realloc(r->hist_rede, novo * sizeof *a);
    if (!a)
        return 0;
    r->hist_rede = a;
    size_t *b = mt_realloc(r->hist_influenciador, novo * sizeof *b);
    if (!b)
        return 0;
    r->hist_influenciador = b;
    memset(a + r->hist_tam, 0, (novo - r->hist_tam) * sizeof *a);
    memset(b + r->hist_tam, 0, (novo - r->hist_tam) * sizeof *b);
    r->hist_tam = novo;
    return 1;
}

/* BFS a partir do perfil p->vertice, preenchendo p e acumulando nos
 * histogramas e somas. */
static SeparacaoStatus analisar_perfil(const Graph *g, SeparacaoResultado *r, SeparacaoPerfil *p,
                                       double *soma_rede, size_t *pares_rede,
                                       double *soma_conjunto, size_t *pares_conjunto)
{
    BfsResultado b;
    if (bfs_executar(g, p->vertice, &b) != BFS_OK)
        return SEPARACAO_ERRO_MEMORIA;
    if (!garantir_hist(r, b.dist_max + 1)) {
        bfs_liberar(&b);
        return SEPARACAO_ERRO_MEMORIA;
    }

    p->alcancados = b.alcancados;
    p->excentricidade = b.dist_max;
    p->dist_media = b.dist_media;
    p->dist_influenciador = SIZE_MAX;
    p->influenciadores_alcancados = 0;
    double soma_inf = 0;

    for (size_t v = 0; v < b.n; v++) {
        if (!b.visitado[v] || v == p->vertice)
            continue;
        size_t d = b.dist[v];
        r->hist_rede[d]++;
        *soma_rede += (double)d;
        (*pares_rede)++;
        if (r->eh_influenciador[v]) {
            p->influenciadores_alcancados++;
            soma_inf += (double)d;
            if (d < p->dist_influenciador)
                p->dist_influenciador = d;
        }
    }
    p->dist_media_influenciadores =
        p->influenciadores_alcancados ? soma_inf / (double)p->influenciadores_alcancados : 0.0;
    *soma_conjunto += soma_inf;
    *pares_conjunto += p->influenciadores_alcancados;

    if (p->dist_influenciador != SIZE_MAX)
        r->hist_influenciador[p->dist_influenciador]++;
    if (b.dist_max > r->diametro_estimado)
        r->diametro_estimado = b.dist_max;
    bfs_liberar(&b);
    return SEPARACAO_OK;
}

SeparacaoStatus separacao_analisar(const Graph *g, double fracao, size_t num_perfis,
                                   unsigned long long semente, SeparacaoResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    if (n == 0 || num_perfis == 0 || !(fracao > 0.0 && fracao <= 1.0))
        return SEPARACAO_ERRO_PARAMETRO;

    SeparacaoStatus st = marcar_influenciadores(g, fracao, r);
    if (st != SEPARACAO_OK) {
        separacao_liberar(r);
        return st;
    }
    if (r->num_influenciadores == n) {   /* ninguém sobrou como perfil comum */
        separacao_liberar(r);
        return SEPARACAO_ERRO_PARAMETRO;
    }

    size_t *sorteados = mt_malloc(num_perfis * sizeof *sorteados);
    if (!sorteados) {
        separacao_liberar(r);
        return SEPARACAO_ERRO_MEMORIA;
    }
    size_t k = sortear_perfis(r, n, num_perfis, semente, sorteados);
    r->perfis = k != SIZE_MAX ? mt_calloc(k, sizeof *r->perfis) : NULL;
    if (!r->perfis) {
        mt_free(sorteados);
        separacao_liberar(r);
        return SEPARACAO_ERRO_MEMORIA;
    }
    r->num_perfis = k;

    double soma_rede = 0, soma_conjunto = 0, soma_inf = 0;
    size_t pares_rede = 0, pares_conjunto = 0, com_inf = 0;
    for (size_t i = 0; i < k; i++) {
        SeparacaoPerfil *p = &r->perfis[i];
        p->vertice = sorteados[i];
        st = analisar_perfil(g, r, p, &soma_rede, &pares_rede, &soma_conjunto, &pares_conjunto);
        if (st != SEPARACAO_OK) {
            mt_free(sorteados);
            separacao_liberar(r);
            return st;
        }
        if (p->dist_influenciador != SIZE_MAX) {
            soma_inf += (double)p->dist_influenciador;
            com_inf++;
        }
    }
    mt_free(sorteados);

    r->perfis_sem_influenciador = k - com_inf;
    r->dist_media_rede = pares_rede ? soma_rede / (double)pares_rede : 0.0;
    r->dist_media_influenciador = com_inf ? soma_inf / (double)com_inf : 0.0;
    r->dist_media_conjunto = pares_conjunto ? soma_conjunto / (double)pares_conjunto : 0.0;
    return SEPARACAO_OK;
}

SeparacaoStatus separacao_exato(const Graph *g, size_t *diametro, double *dist_media)
{
    size_t n = graph_num_vertices(g);
    *diametro = 0;
    *dist_media = 0.0;
    double soma = 0;
    size_t pares = 0;
    for (size_t s = 0; s < n; s++) {
        BfsResultado b;
        if (bfs_executar(g, s, &b) != BFS_OK)
            return SEPARACAO_ERRO_MEMORIA;
        for (size_t v = 0; v < n; v++)
            if (b.visitado[v] && v != s) {
                soma += (double)b.dist[v];
                pares++;
            }
        if (b.dist_max > *diametro)
            *diametro = b.dist_max;
        bfs_liberar(&b);
    }
    *dist_media = pares ? soma / (double)pares : 0.0;
    return SEPARACAO_OK;
}
