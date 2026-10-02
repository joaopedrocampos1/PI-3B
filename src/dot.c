#include "dot.h"
#include "memtrack.h"

#include <stdio.h>
#include <stdlib.h>

static int comparar_arestas(const void *a, const void *b)
{
    const DotAresta *x = a, *y = b;
    if (x->u != y->u)
        return x->u < y->u ? -1 : 1;
    return (x->v > y->v) - (x->v < y->v);
}

/* Cópia ordenada das arestas destacadas, para busca binária. Nas visões sem
 * sentido, cada par sai com a ponta menor primeiro. */
static DotAresta *preparar_destaques(const DotEstilo *e, int sem_sentido)
{
    DotAresta *d = mt_malloc((e->num_arestas ? e->num_arestas : 1) * sizeof *d);
    if (!d)
        return NULL;
    for (size_t i = 0; i < e->num_arestas; i++) {
        d[i] = e->arestas[i];
        if (sem_sentido && d[i].u > d[i].v) {
            size_t t = d[i].u;
            d[i].u = d[i].v;
            d[i].v = t;
        }
    }
    qsort(d, e->num_arestas, sizeof *d, comparar_arestas);
    return d;
}

/* Texto entre aspas no formato .dot: aspas, barras e quebras escapadas. */
static void escrever_texto(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) {
        if (*s == '"' || *s == '\\')
            fputc('\\', f);
        if (*s == '\n')
            fputs("\\n", f);
        else
            fputc(*s, f);
    }
    fputc('"', f);
}

DotStatus dot_gravar(const char *caminho, const Graph *g, const IdMap *ids,
                     const DotEstilo *estilo, const char *titulo)
{
    static const DotEstilo VAZIO = {NULL, NULL, NULL, 0};
    const DotEstilo *e = estilo ? estilo : &VAZIO;
    int sem_sentido = graph_view(g) != GRAPH_DIRECTED;
    size_t n = graph_num_vertices(g);

    DotAresta *destaques = preparar_destaques(e, sem_sentido);
    if (!destaques)
        return DOT_ERRO_MEMORIA;
    FILE *f = fopen(caminho, "w");
    if (!f) {
        mt_free(destaques);
        return DOT_ERRO_ARQUIVO;
    }

    fprintf(f, "%s G {\n", sem_sentido ? "graph" : "digraph");
    /* Acima de algumas dezenas de vértices, o layout em camadas (o padrão do
     * Graphviz) fica ilegível e lento numa rede densa; o sfdp, por forças, não.
     * Declarado no arquivo, vale com o comando comum: dot -Tpng arq.dot */
    if (n > DOT_LIMITE_CAMADAS)
        fputs("  layout=sfdp;\n  overlap=prism;\n", f);
    if (titulo) {
        fputs("  label=", f);
        escrever_texto(f, titulo);
        fputs(";\n  labelloc=t;\n", f);
    }
    fputs("  node [shape=circle, fontsize=9, fontname=\"Helvetica\", color=\"#52514e\"];\n", f);
    fputs("  edge [color=\"#8a8984\", arrowsize=0.6];\n", f);

    /* todos os vértices, inclusive os sem arestas */
    for (size_t v = 0; v < n; v++) {
        fprintf(f, "  \"%llu\"", idmap_original(ids, v));
        int cor = e->cor && e->cor[v];
        int destaque = e->destaque && e->destaque[v];
        if (cor || destaque) {
            fputs(" [", f);
            if (cor)
                fprintf(f, "style=filled, fillcolor=\"%s\"%s", e->cor[v], destaque ? ", " : "");
            if (destaque)
                fputs("penwidth=3, color=\"" DOT_COR_DESTAQUE "\"", f);
            fputc(']', f);
        }
        fputs(";\n", f);
    }

    const char *seta = sem_sentido ? "--" : "->";
    for (size_t u = 0; u < n; u++) {
        GraphIter it;
        size_t w;
        graph_neighbors_begin(g, u, &it);
        while (graph_neighbors_next(&it, &w)) {
            if (sem_sentido && w < u)
                continue;   /* {u, w} sai uma vez só */
            DotAresta chave = {u, w};
            int destacada = bsearch(&chave, destaques, e->num_arestas, sizeof *destaques,
                                    comparar_arestas) != NULL;
            fprintf(f, "  \"%llu\" %s \"%llu\"%s;\n", idmap_original(ids, u), seta,
                    idmap_original(ids, w),
                    destacada ? " [color=\"" DOT_COR_DESTAQUE "\", penwidth=2.5]" : "");
        }
    }
    fputs("}\n", f);

    mt_free(destaques);
    int erro = ferror(f);
    if (fclose(f) != 0 || erro)
        return DOT_ERRO_ARQUIVO;
    return DOT_OK;
}
