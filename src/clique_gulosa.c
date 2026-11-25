#include "clique.h"
#include "memtrack.h"

#include <string.h>

/* Heurística gulosa da Clique Máxima (#68). A descrição está em clique.h. */

/* Ordena os vértices por grau decrescente e, no empate, por índice crescente:
 * contagem por grau, estável, de V + grau máximo passos. `cont` tem
 * grau_max + 1 posições. */
static void ordenar_por_grau(const Graph *g, size_t n, size_t grau_max, size_t *ordem,
                             size_t *cont)
{
    memset(cont, 0, (grau_max + 1) * sizeof *cont);
    for (size_t v = 0; v < n; v++)
        cont[graph_degree(g, v)]++;
    /* cont[d] passa a ser a primeira posição dos vértices de grau d, com os
     * graus maiores na frente */
    size_t pos = 0;
    for (size_t d = grau_max + 1; d-- > 0;) {
        size_t c = cont[d];
        cont[d] = pos;
        pos += c;
    }
    for (size_t v = 0; v < n; v++)
        ordem[cont[graph_degree(g, v)]++] = v;
}

static void ordenar_crescente(size_t *v, size_t m)
{
    for (size_t a = 1; a < m; a++) {
        size_t x = v[a], b = a;
        while (b > 0 && v[b - 1] > x) {
            v[b] = v[b - 1];
            b--;
        }
        v[b] = x;
    }
}

/* Clique gulosa a partir de `s`, gravada em `atual`. `cand` tem espaço para
 * grau(s) vértices. Devolve o tamanho da clique. */
static size_t a_partir_de(const Graph *g, size_t s, size_t *atual, size_t *cand)
{
    size_t tam = 0, m = 0;
    atual[tam++] = s;

    /* candidatos: os vizinhos de todos os que já estão na clique, sempre em
     * ordem crescente de índice */
    GraphIter it;
    size_t w;
    graph_neighbors_begin(g, s, &it);
    while (graph_neighbors_next(&it, &w))
        cand[m++] = w;

    while (m > 0) {
        /* o de maior grau; com a desigualdade estrita, no empate fica o
         * primeiro, que é o de menor índice */
        size_t escolhido = 0;
        for (size_t j = 1; j < m; j++)
            if (graph_degree(g, cand[j]) > graph_degree(g, cand[escolhido]))
                escolhido = j;
        size_t c = cand[escolhido];
        atual[tam++] = c;

        size_t k = 0;
        for (size_t j = 0; j < m; j++)
            if (j != escolhido && graph_has_edge(g, c, cand[j]))
                cand[k++] = cand[j];
        m = k;
    }
    return tam;
}

CliqueStatus clique_gulosa(const Graph *g, size_t inicios, CliqueResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    r->n = n;
    if (n == 0)
        return CLIQUE_OK;
    size_t k = (inicios == 0 || inicios > n) ? n : inicios;

    size_t grau_max = 0;
    for (size_t v = 0; v < n; v++) {
        size_t d = graph_degree(g, v);
        if (d > grau_max)
            grau_max = d;
    }
    /* uma clique tem no máximo grau_max + 1 vértices */
    size_t g1 = grau_max + 1;
    size_t *ordem = mt_malloc(n * sizeof *ordem);
    size_t *cont = mt_malloc(g1 * sizeof *cont);
    size_t *cand = mt_malloc(g1 * sizeof *cand);
    size_t *atual = mt_malloc(g1 * sizeof *atual);
    size_t *melhor = mt_malloc(g1 * sizeof *melhor);

    int ok = ordem && cont && cand && atual && melhor;
    size_t tam_melhor = 0;
    if (ok) {
        ordenar_por_grau(g, n, grau_max, ordem, cont);
        for (size_t i = 0; i < k; i++) {
            size_t tam = a_partir_de(g, ordem[i], atual, cand);
            r->nos += tam;
            if (tam > tam_melhor) {
                memcpy(melhor, atual, tam * sizeof *atual);
                tam_melhor = tam;
            }
        }
        r->clique = mt_malloc(tam_melhor * sizeof *r->clique);
        ok = r->clique != NULL;
    }
    if (ok) {
        memcpy(r->clique, melhor, tam_melhor * sizeof *melhor);
        ordenar_crescente(r->clique, tam_melhor);
        r->tam = tam_melhor;
    }

    mt_free(melhor);
    mt_free(atual);
    mt_free(cand);
    mt_free(cont);
    mt_free(ordem);
    if (!ok) {
        mt_free(r->clique);
        memset(r, 0, sizeof *r);
        return CLIQUE_ERRO_MEMORIA;
    }
    return CLIQUE_OK;
}
