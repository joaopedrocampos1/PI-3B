#include "componentes.h"
#include "memtrack.h"

#include <stdint.h>
#include <string.h>

/* Um nível da pilha explícita que substitui a recursão da DFS: o vértice e
 * até onde seus vizinhos já foram percorridos. */
typedef struct {
    size_t v;
    GraphIter it;
} Quadro;

/* Vetores de trabalho do Tarjan; só existem durante componentes_executar. */
typedef struct {
    size_t *indice;           /* ordem de descoberta, a partir de 1; 0 = não visitado */
    size_t *low;              /* menor índice alcançável pela subárvore do vértice */
    unsigned char *na_pilha;  /* 1 se o vértice está na pilha de Tarjan */
    size_t *pilha;            /* pilha de Tarjan: vértices ainda sem componente */
    Quadro *quadros;          /* pilha da DFS */
} Trabalho;

static void liberar_trabalho(Trabalho *t)
{
    mt_free(t->indice);
    mt_free(t->low);
    mt_free(t->na_pilha);
    mt_free(t->pilha);
    mt_free(t->quadros);
}

static void descobrir(const Graph *g, Trabalho *t, size_t v, size_t *contador,
                      size_t *topo_pilha, size_t *topo_quadros)
{
    t->indice[v] = t->low[v] = ++*contador;
    t->pilha[(*topo_pilha)++] = v;
    t->na_pilha[v] = 1;

    Quadro *q = &t->quadros[(*topo_quadros)++];
    q->v = v;
    graph_neighbors_begin(g, v, &q->it);
}

ComponentesStatus componentes_executar(const Graph *g, ComponentesResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    size_t alocar = n ? n : 1;

    Trabalho t;
    t.indice = mt_calloc(alocar, sizeof *t.indice);
    t.low = mt_malloc(alocar * sizeof *t.low);
    t.na_pilha = mt_calloc(alocar, 1);
    t.pilha = mt_malloc(alocar * sizeof *t.pilha);
    t.quadros = mt_malloc(alocar * sizeof *t.quadros);
    r->componente = mt_malloc(alocar * sizeof *r->componente);
    r->tamanho = mt_malloc(alocar * sizeof *r->tamanho);   /* no máximo V componentes */
    if (!t.indice || !t.low || !t.na_pilha || !t.pilha || !t.quadros || !r->componente ||
        !r->tamanho) {
        liberar_trabalho(&t);
        componentes_liberar(r);
        return COMPONENTES_ERRO_MEMORIA;
    }
    r->n = n;

    size_t contador = 0, topo_pilha = 0, topo_quadros = 0;
    for (size_t raiz = 0; raiz < n; raiz++) {
        if (t.indice[raiz] != 0)
            continue;
        descobrir(g, &t, raiz, &contador, &topo_pilha, &topo_quadros);

        while (topo_quadros > 0) {
            Quadro *q = &t.quadros[topo_quadros - 1];
            size_t v = q->v, w;

            if (graph_neighbors_next(&q->it, &w)) {
                if (t.indice[w] == 0)
                    descobrir(g, &t, w, &contador, &topo_pilha, &topo_quadros);
                else if (t.na_pilha[w] && t.indice[w] < t.low[v])
                    t.low[v] = t.indice[w];
                continue;
            }

            /* vizinhos de v esgotados: "retorno" da chamada recursiva */
            topo_quadros--;
            if (topo_quadros > 0) {
                size_t pai = t.quadros[topo_quadros - 1].v;
                if (t.low[v] < t.low[pai])
                    t.low[pai] = t.low[v];
            }

            /* v é a raiz de um componente: ele e quem está acima dele na pilha */
            if (t.low[v] == t.indice[v]) {
                size_t c = r->num_componentes++, tam = 0, x;
                do {
                    x = t.pilha[--topo_pilha];
                    t.na_pilha[x] = 0;
                    r->componente[x] = c;
                    tam++;
                } while (x != v);
                r->tamanho[c] = tam;
            }
        }
    }

    liberar_trabalho(&t);

    r->gigante = n ? 0 : SIZE_MAX;
    for (size_t c = 0; c < r->num_componentes; c++) {
        if (r->tamanho[c] > r->tamanho[r->gigante])
            r->gigante = c;
        r->unitarios += r->tamanho[c] == 1;
    }
    r->tamanho_gigante = n ? r->tamanho[r->gigante] : 0;
    return COMPONENTES_OK;
}

ComponentesStatus componentes_distribuicao(const ComponentesResultado *r,
                                           ComponentesFaixa **faixas, size_t *k)
{
    *faixas = NULL;
    *k = 0;

    /* tamanhos vão de 1 a V: contagem direta, sem ordenar */
    size_t *quantos = mt_calloc(r->n + 1, sizeof *quantos);
    if (!quantos)
        return COMPONENTES_ERRO_MEMORIA;
    for (size_t c = 0; c < r->num_componentes; c++)
        if (quantos[r->tamanho[c]]++ == 0)
            (*k)++;

    *faixas = mt_malloc((*k ? *k : 1) * sizeof **faixas);
    if (!*faixas) {
        mt_free(quantos);
        *k = 0;
        return COMPONENTES_ERRO_MEMORIA;
    }
    size_t i = 0;
    for (size_t tam = r->n; tam >= 1; tam--)
        if (quantos[tam]) {
            (*faixas)[i].tamanho = tam;
            (*faixas)[i].quantidade = quantos[tam];
            i++;
        }

    mt_free(quantos);
    return COMPONENTES_OK;
}

void componentes_liberar(ComponentesResultado *r)
{
    mt_free(r->componente);
    mt_free(r->tamanho);
    memset(r, 0, sizeof *r);
}
