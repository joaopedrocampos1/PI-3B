#include "dfs.h"
#include "memtrack.h"

#include <string.h>

/* Estrutura para a pilha explícita da DFS */
typedef struct {
    size_t v;       /* Vértice atual */
    GraphIter it;   /* Guardador de posição dos vizinhos do vértice v */
} DfsFrame;

/* Função principal da DFS Iterativa */
int graph_dfs_iterativa(const Graph *g, size_t inicial, size_t *descoberta, size_t *finalizacao, size_t *tempo) {
    size_t n = graph_num_vertices(g);
    
    // Aloca a pilha explicitamente usando o rastreador de memória do grupo
    DfsFrame *pilha = mt_malloc(n * sizeof(DfsFrame));
    if (!pilha) return 0;

    int topo = -1;

    // Empilha o vértice inicial
    topo++;
    pilha[topo].v = inicial;
    graph_neighbors_begin(g, inicial, &(pilha[topo].it));
    
    (*tempo)++;
    descoberta[inicial] = *tempo;

    while (topo >= 0) {
        size_t u = pilha[topo].v;
        size_t w;
        int tem_vizinho = graph_neighbors_next(&(pilha[topo].it), &w);

        if (tem_vizinho) {
            // Se o vizinho 'w' ainda não foi descoberto
            if (descoberta[w] == 0) {
                topo++;
                pilha[topo].v = w;
                graph_neighbors_begin(g, w, &(pilha[topo].it));
                
                (*tempo)++;
                descoberta[w] = *tempo;
            }
        } else {
            // Todos os vizinhos de 'u' foram explorados, então desempilha e finaliza
            (*tempo)++;
            finalizacao[u] = *tempo;
            topo--;
        }
    }

    mt_free(pilha);
    return 1;
}

/* Floresta da DFS: a primeira árvore parte da origem e as seguintes dos
 * vértices ainda não descobertos, em ordem de índice, todas com o mesmo relógio. */
DfsStatus dfs_executar(const Graph *g, size_t origem, DfsResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    if (origem >= n)
        return DFS_ERRO_ORIGEM;

    r->n = n;
    r->descoberta = mt_calloc(n, sizeof *r->descoberta);
    r->finalizacao = mt_calloc(n, sizeof *r->finalizacao);
    if (!r->descoberta || !r->finalizacao) {
        dfs_liberar(r);
        return DFS_ERRO_MEMORIA;
    }

    size_t tempo = 0;
    for (size_t k = 0; k <= n; k++) {
        /* k = 0 é a origem; depois, os vértices em ordem, pulando os já vistos */
        size_t v = k == 0 ? origem : k - 1;
        if (r->descoberta[v] != 0)
            continue;
        size_t antes = tempo;
        if (!graph_dfs_iterativa(g, v, r->descoberta, r->finalizacao, &tempo)) {
            dfs_liberar(r);
            return DFS_ERRO_MEMORIA;
        }
        if (r->arvores++ == 0)
            r->alcancados = (tempo - antes) / 2;   /* cada vértice gera dois eventos */
    }
    return DFS_OK;
}

void dfs_liberar(DfsResultado *r)
{
    mt_free(r->descoberta);
    mt_free(r->finalizacao);
    memset(r, 0, sizeof *r);
}
