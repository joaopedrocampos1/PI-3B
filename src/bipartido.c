#include "bipartido.h"
#include "memtrack.h"

#include <stdint.h>
#include <string.h>

/* Reconstrói o ciclo ímpar fechado pela aresta {u, w}, com u e w da mesma
 * cor na mesma árvore de BFS: sobe das duas pontas até o ancestral comum.
 * O ciclo sai como ancestral -> ... -> u, w -> ... -> filho do ancestral.
 *
 * Num BFS, as pontas de uma aresta ficam no máximo a um nível de distância;
 * com a mesma cor, têm a mesma paridade, então estão no mesmo nível. Por isso
 * dá para subir as duas juntas, sem igualar profundidades antes. */
static int montar_ciclo(BipartidoResultado *r, const size_t *pai, const size_t *prof, size_t u,
                        size_t w)
{
    size_t a = u, b = w;
    while (a != b) {
        a = pai[a];
        b = pai[b];
    }
    size_t lca = a;

    size_t lado_u = prof[u] - prof[lca] + 1;   /* de lca até u, inclusive */
    size_t lado_w = prof[w] - prof[lca];       /* de w até o filho de lca */
    r->tam_ciclo = lado_u + lado_w;
    r->ciclo_impar = mt_malloc(r->tam_ciclo * sizeof *r->ciclo_impar);
    if (!r->ciclo_impar)
        return 0;

    size_t x = u, i = lado_u - 1;
    for (;;) {
        r->ciclo_impar[i] = x;
        if (i == 0)
            break;
        i--;
        x = pai[x];
    }
    i = lado_u;
    for (x = w; x != lca; x = pai[x])
        r->ciclo_impar[i++] = x;
    return 1;
}

BipartidoStatus bipartido_executar(const Graph *g, BipartidoResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    size_t alocar = n ? n : 1;

    unsigned char *visto = mt_calloc(alocar, 1);
    size_t *pai = mt_malloc(alocar * sizeof *pai);
    size_t *prof = mt_malloc(alocar * sizeof *prof);   /* profundidade na árvore do BFS */
    size_t *fila = mt_malloc(alocar * sizeof *fila);
    r->cor = mt_calloc(alocar, 1);
    int ok = visto && pai && prof && fila && r->cor;

    r->n = n;
    r->bipartido = 1;
    size_t conflito_u = SIZE_MAX, conflito_w = SIZE_MAX;

    for (size_t raiz = 0; ok && raiz < n; raiz++) {
        if (visto[raiz])
            continue;
        visto[raiz] = 1;
        r->cor[raiz] = 0;
        pai[raiz] = raiz;
        prof[raiz] = 0;
        size_t ini = 0, fim = 0;
        fila[fim++] = raiz;

        while (ini < fim) {
            size_t u = fila[ini++], w;
            GraphIter it;
            graph_neighbors_begin(g, u, &it);
            while (graph_neighbors_next(&it, &w)) {
                if (!visto[w]) {
                    visto[w] = 1;
                    r->cor[w] = (unsigned char)(1 - r->cor[u]);
                    pai[w] = u;
                    prof[w] = prof[u] + 1;
                    fila[fim++] = w;
                } else if (r->cor[w] == r->cor[u] && r->bipartido) {
                    /* primeiro conflito: guarda para o exemplo e segue colorindo */
                    r->bipartido = 0;
                    conflito_u = u;
                    conflito_w = w;
                }
            }
        }
    }

    if (ok) {
        for (size_t v = 0; v < n; v++)
            r->lado[r->cor[v]]++;
        if (!r->bipartido)
            ok = montar_ciclo(r, pai, prof, conflito_u, conflito_w);
    }

    mt_free(visto);
    mt_free(pai);
    mt_free(prof);
    mt_free(fila);
    if (!ok) {
        bipartido_liberar(r);
        return BIPARTIDO_ERRO_MEMORIA;
    }
    return BIPARTIDO_OK;
}

void bipartido_liberar(BipartidoResultado *r)
{
    mt_free(r->cor);
    mt_free(r->ciclo_impar);
    memset(r, 0, sizeof *r);
}
