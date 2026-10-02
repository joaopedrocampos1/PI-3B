#include "../include/graph.h"
#include "memtrack.h"
#include <stdlib.h>

/* Estrutura para a pilha explícita da DFS */
typedef struct {
    size_t v;       /* Vértice atual */
    GraphIter it;   /* Guardador de posição dos vizinhos do vértice v */
} DfsFrame;

/* Função principal da DFS Iterativa */
void graph_dfs_iterativa(const Graph *g, size_t inicial, size_t *descoberta, size_t *finalizacao, size_t *tempo) {
    size_t n = graph_num_vertices(g);
    
    // Aloca a pilha explicitamente usando o rastreador de memória do grupo
    DfsFrame *pilha = mt_malloc(n * sizeof(DfsFrame));
    if (!pilha) return;

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
}
