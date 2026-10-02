#include "tarjan.h"
#include "memtrack.h"

#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Estrutura para a pilha explícita do Tarjan para evitar Stack Overflow */
typedef struct {
    size_t u;
    size_t pai;
    size_t filhos;
    GraphIter it;
    int estado; /* 0: primeira visita, 1: processando vizinhos */
} TarjanFrame;

/* Vetores de trabalho, com V posições cada; só existem durante tarjan_executar. */
typedef struct {
    size_t *num;        /* ordem de descoberta, a partir de 1; 0 = não visitado */
    size_t *low;
    size_t *sub;        /* tamanho da subárvore da DFS */
    size_t *raiz;       /* raiz da árvore da DFS do vértice: o tamanho do componente é sub[raiz] */
    size_t *soma_sep;   /* vértices das subárvores que se separam do vértice sem ele */
    size_t *maior_sep;  /* a maior dessas subárvores */
    size_t *qtd_sep;    /* quantas são */
    TarjanFrame *pilha;
} Trabalho;

static void liberar_trabalho(Trabalho *t)
{
    mt_free(t->num);
    mt_free(t->low);
    mt_free(t->sub);
    mt_free(t->raiz);
    mt_free(t->soma_sep);
    mt_free(t->maior_sep);
    mt_free(t->qtd_sep);
    mt_free(t->pilha);
}

/* Impacto decrescente; em empate, índice crescente. */
static int comparar_impacto(const void *a, const void *b)
{
    const Articulacao *x = a, *y = b;
    if (x->impacto != y->impacto)
        return x->impacto > y->impacto ? -1 : 1;
    return (x->v > y->v) - (x->v < y->v);
}

/* Pedaços e impacto de cada articulação, a partir das subárvores separadas,
 * e ranking por impacto. */
static int ranquear(TarjanResultado *r, const Trabalho *t)
{
    r->articulacoes = mt_malloc((r->num_articulacoes ? r->num_articulacoes : 1) *
                                sizeof *r->articulacoes);
    if (!r->articulacoes)
        return 0;

    size_t k = 0;
    for (size_t v = 0; v < r->n; v++) {
        if (!r->eh_articulacao[v])
            continue;
        size_t tam_componente = t->sub[t->raiz[v]];
        int eh_raiz = t->raiz[v] == v;
        /* fora a raiz, o que não está nas subárvores separadas vira mais um pedaço */
        size_t resto = eh_raiz ? 0 : tam_componente - 1 - t->soma_sep[v];
        size_t maior = MAX(t->maior_sep[v], resto);

        r->articulacoes[k].v = v;
        r->articulacoes[k].pedacos = t->qtd_sep[v] + (eh_raiz ? 0 : 1);
        r->articulacoes[k].impacto = tam_componente - 1 - maior;
        k++;
    }
    qsort(r->articulacoes, k, sizeof *r->articulacoes, comparar_impacto);
    return 1;
}

/* Encontra pontes e vértices de articulação (pontes de informação) */
TarjanStatus tarjan_executar(const Graph *g, TarjanResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    size_t alocar = n ? n : 1;

    Trabalho t;
    t.num = mt_calloc(alocar, sizeof *t.num);
    t.low = mt_malloc(alocar * sizeof *t.low);
    t.sub = mt_malloc(alocar * sizeof *t.sub);
    t.raiz = mt_malloc(alocar * sizeof *t.raiz);
    t.soma_sep = mt_calloc(alocar, sizeof *t.soma_sep);
    t.maior_sep = mt_calloc(alocar, sizeof *t.maior_sep);
    t.qtd_sep = mt_calloc(alocar, sizeof *t.qtd_sep);
    t.pilha = mt_malloc(alocar * sizeof *t.pilha);
    r->eh_articulacao = mt_calloc(alocar, 1);
    r->pontes = mt_malloc(alocar * sizeof *r->pontes);   /* no máximo V - 1 pontes */
    if (!t.num || !t.low || !t.sub || !t.raiz || !t.soma_sep || !t.maior_sep || !t.qtd_sep ||
        !t.pilha || !r->eh_articulacao || !r->pontes) {
        liberar_trabalho(&t);
        tarjan_liberar(r);
        return TARJAN_ERRO_MEMORIA;
    }
    r->n = n;

    size_t tempo = 0;
    for (size_t i = 0; i < n; i++) {
        if (t.num[i] != 0)
            continue;

        // Inicia uma nova árvore do DFS para componentes desconexos
        size_t topo = 0;
        t.pilha[topo].u = i;
        t.pilha[topo].pai = n; // Valor sentinela significando "sem pai"
        t.pilha[topo].filhos = 0;
        t.pilha[topo].estado = 0;
        graph_neighbors_begin(g, i, &t.pilha[topo].it);

        for (;;) {
            TarjanFrame *q = &t.pilha[topo];
            size_t u = q->u;

            if (q->estado == 0) {
                t.num[u] = t.low[u] = ++tempo;
                t.sub[u] = 1;
                t.raiz[u] = i;
                q->estado = 1;
            }

            size_t v;
            if (graph_neighbors_next(&q->it, &v)) {
                if (v == q->pai)
                    continue; // Ignora a aresta que veio do pai direto

                if (t.num[v] != 0) {
                    // Aresta de retorno: atualiza o low de 'u'
                    t.low[u] = MIN(t.low[u], t.num[v]);
                } else {
                    // Aresta de árvore: incrementa os filhos e empilha o vizinho 'v'
                    q->filhos++;
                    topo++;
                    t.pilha[topo].u = v;
                    t.pilha[topo].pai = u;
                    t.pilha[topo].filhos = 0;
                    t.pilha[topo].estado = 0;
                    graph_neighbors_begin(g, v, &t.pilha[topo].it);
                }
                continue;
            }

            // Todos os vizinhos processados: faz o backtracking para o pai
            if (topo == 0) {
                // Condição de articulação para a raiz da árvore DFS
                if (q->filhos > 1)
                    r->eh_articulacao[u] = 1;
                break;
            }
            size_t p = t.pilha[topo - 1].u;
            t.low[p] = MIN(t.low[p], t.low[u]);
            t.sub[p] += t.sub[u];

            if (t.low[u] > t.num[p]) {
                // Nenhuma aresta de retorno passa por cima de {p, u}: é ponte
                r->pontes[r->num_pontes].u = MIN(p, u);
                r->pontes[r->num_pontes].v = MAX(p, u);
                r->num_pontes++;
            }
            if (t.low[u] >= t.num[p]) {
                // Sem p, a subárvore de u fica separada do resto
                t.soma_sep[p] += t.sub[u];
                t.maior_sep[p] = MAX(t.maior_sep[p], t.sub[u]);
                t.qtd_sep[p]++;
                // Condição de articulação para vértices que não são a raiz da árvore DFS
                if (t.pilha[topo - 1].pai != n)
                    r->eh_articulacao[p] = 1;
            }
            topo--;
        }
    }

    for (size_t v = 0; v < n; v++)
        r->num_articulacoes += r->eh_articulacao[v];
    int ok = ranquear(r, &t);

    liberar_trabalho(&t);
    if (!ok) {
        tarjan_liberar(r);
        return TARJAN_ERRO_MEMORIA;
    }
    return TARJAN_OK;
}

void tarjan_liberar(TarjanResultado *r)
{
    mt_free(r->eh_articulacao);
    mt_free(r->articulacoes);
    mt_free(r->pontes);
    memset(r, 0, sizeof *r);
}
