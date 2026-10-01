#include "subgraph.h"

#include <stdlib.h>
#include <string.h>

static int comparar_indices(const void *a, const void *b)
{
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x > y) - (x < y);
}

/* Ordena a lista de vizinhos de cada vértice e remove repetições (um par que
 * se segue mutuamente aparece duas vezes). Compacta o vetor no lugar. */
static void ordenar_sem_repeticao(Amostrador *a)
{
    size_t V = a->ids.n, escrita = 0, ini = 0;
    for (size_t v = 0; v < V; v++) {
        size_t fim = a->inicio[v + 1];
        qsort(a->vizinhos + ini, fim - ini, sizeof *a->vizinhos, comparar_indices);

        a->inicio[v] = escrita;
        for (size_t j = ini; j < fim; j++)
            if (escrita == a->inicio[v] || a->vizinhos[j] != a->vizinhos[escrita - 1])
                a->vizinhos[escrita++] = a->vizinhos[j];
        ini = fim;
    }
    a->inicio[V] = escrita;
}

AmostraStatus subgraph_preparar(Amostrador *a, const EdgeList *el)
{
    memset(a, 0, sizeof *a);
    a->arestas = el;
    idmap_iniciar(&a->ids);

    size_t u, v;
    for (size_t e = 0; e < el->n; e++)
        if (!idmap_inserir(&a->ids, el->arestas[e].origem, &u) ||
            !idmap_inserir(&a->ids, el->arestas[e].destino, &v))
            goto sem_memoria;

    size_t V = a->ids.n;
    a->inicio = calloc(V + 1, sizeof *a->inicio);
    a->vizinhos = malloc((2 * el->n + 1) * sizeof *a->vizinhos);
    size_t *cursor = malloc((V + 1) * sizeof *cursor);
    if (!a->inicio || !a->vizinhos || !cursor) {
        free(cursor);
        goto sem_memoria;
    }

    /* conta quantos vizinhos cada vértice tem, em inicio[v + 1] */
    for (size_t e = 0; e < el->n; e++) {
        idmap_buscar(&a->ids, el->arestas[e].origem, &u);
        idmap_buscar(&a->ids, el->arestas[e].destino, &v);
        a->inicio[u + 1]++;
        a->inicio[v + 1]++;
    }
    for (size_t k = 0; k < V; k++)
        a->inicio[k + 1] += a->inicio[k];

    /* cada aresta entra nos dois sentidos */
    memcpy(cursor, a->inicio, (V + 1) * sizeof *cursor);
    for (size_t e = 0; e < el->n; e++) {
        idmap_buscar(&a->ids, el->arestas[e].origem, &u);
        idmap_buscar(&a->ids, el->arestas[e].destino, &v);
        a->vizinhos[cursor[u]++] = v;
        a->vizinhos[cursor[v]++] = u;
    }
    free(cursor);

    ordenar_sem_repeticao(a);
    return AMOSTRA_OK;

sem_memoria:
    subgraph_liberar(a);
    return AMOSTRA_ERRO_MEMORIA;
}

/* Copia para `saida` as arestas originais com as duas pontas marcadas. */
static AmostraStatus extrair_induzido(const Amostrador *a, const unsigned char *marcado,
                                      EdgeList *saida)
{
    const EdgeList *el = a->arestas;
    size_t u, v, total = 0;

    for (int passo = 0; passo < 2; passo++) {
        if (passo == 1) {
            saida->arestas = malloc((total ? total : 1) * sizeof *saida->arestas);
            if (!saida->arestas)
                return AMOSTRA_ERRO_MEMORIA;
        }
        for (size_t e = 0; e < el->n; e++) {
            idmap_buscar(&a->ids, el->arestas[e].origem, &u);
            idmap_buscar(&a->ids, el->arestas[e].destino, &v);
            if (!marcado[u] || !marcado[v])
                continue;
            if (passo == 0)
                total++;
            else
                saida->arestas[saida->n++] = el->arestas[e];
        }
    }
    return AMOSTRA_OK;
}

AmostraStatus subgraph_bfs(const Amostrador *a, unsigned long long semente, size_t n,
                           EdgeList *saida)
{
    memset(saida, 0, sizeof *saida);

    size_t s;
    if (!idmap_buscar(&a->ids, semente, &s))
        return AMOSTRA_SEMENTE_INEXISTENTE;
    if (n == 0)
        return AMOSTRA_OK;

    size_t V = a->ids.n;
    unsigned char *marcado = calloc(V, 1);
    size_t *fila = malloc(n * sizeof *fila);
    if (!marcado || !fila) {
        free(marcado);
        free(fila);
        return AMOSTRA_ERRO_MEMORIA;
    }

    /* um vértice entra na amostra quando é descoberto (entra na fila) */
    size_t descobertos = 0, frente = 0;
    marcado[s] = 1;
    fila[descobertos++] = s;
    while (frente < descobertos && descobertos < n) {
        size_t u = fila[frente++];
        for (size_t j = a->inicio[u]; j < a->inicio[u + 1] && descobertos < n; j++) {
            size_t w = a->vizinhos[j];
            if (!marcado[w]) {
                marcado[w] = 1;
                fila[descobertos++] = w;
            }
        }
    }

    AmostraStatus st = descobertos < n ? AMOSTRA_COMPONENTE_PEQUENO
                                       : extrair_induzido(a, marcado, saida);
    if (st != AMOSTRA_OK)
        edgelist_liberar(saida);
    free(marcado);
    free(fila);
    return st;
}

/* Gerador splitmix64. Próprio em vez de rand(): rand() muda de sequência
 * entre bibliotecas C, e a mesma semente geraria amostras diferentes em
 * Linux e Windows. */
static unsigned long long proximo_aleatorio(unsigned long long *estado)
{
    unsigned long long z = (*estado += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

AmostraStatus subgraph_sortear_sementes(const Amostrador *a, unsigned long long semente_rng,
                                        size_t k, unsigned long long *sementes)
{
    size_t V = a->ids.n;
    if (k > V)
        return AMOSTRA_SEMENTES_INSUFICIENTES;

    size_t *perm = malloc((V ? V : 1) * sizeof *perm);
    if (!perm)
        return AMOSTRA_ERRO_MEMORIA;
    for (size_t v = 0; v < V; v++)
        perm[v] = v;

    /* Fisher-Yates parcial: k trocas sorteiam k vértices distintos */
    unsigned long long estado = semente_rng;
    for (size_t i = 0; i < k; i++) {
        size_t j = i + (size_t)(proximo_aleatorio(&estado) % (V - i));
        size_t t = perm[i];
        perm[i] = perm[j];
        perm[j] = t;
        sementes[i] = idmap_original(&a->ids, perm[i]);
    }

    free(perm);
    return AMOSTRA_OK;
}

void subgraph_liberar(Amostrador *a)
{
    idmap_liberar(&a->ids);
    free(a->inicio);
    free(a->vizinhos);
    memset(a, 0, sizeof *a);
}
