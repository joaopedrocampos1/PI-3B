#include "../include/graph.h"
#include "memtrack.h"
#include <stdio.h>
#include <stdlib.h>

/* Estados dos vértices para a detecção de ciclos */
#define BRANCO 0 /* Não visitado */
#define CINZA  1 /* Na pilha de recursão/exploração (ativo) */
#define PRETO  2 /* Totalmente explorado */

typedef struct {
    size_t v;
    GraphIter it;
} CycleFrame;

/* Função para rastrear e imprimir o ciclo a partir da pilha */
static void reportar_ciclo(CycleFrame *pilha, int topo, size_t destino) {
    printf("Ciclo detectado: ");
    int i = 0;
    // Encontra onde o ciclo começa na pilha
    while (i <= topo && pilha[i].v != destino) {
        i++;
    }
    // Imprime o caminho do ciclo
    for (int j = i; j <= topo; j++) {
        printf("%zu -> ", pilha[j].v);
    }
    printf("%zu\n", destino);
}

/* Algoritmo de detecção de ciclos com pilha explícita */
int graph_detectar_ciclos(const Graph *g) {
    size_t n = graph_num_vertices(g);
    int *estado = mt_calloc(n, sizeof(int));
    CycleFrame *pilha = mt_malloc(n * sizeof(CycleFrame));
    
    if (!estado || !pilha) {
        if (estado) mt_free(estado);
        if (pilha) mt_free(pilha);
        return 0;
    }

    int encontrou_ciclo = 0;

    // Percorre todos os vértices para cobrir grafos desconexos
    for (size_t start = 0; start < n && !encontrou_ciclo; start++) {
        if (estado[start] != BRANCO) continue;

        int topo = 0;
        pilha[topo].v = start;
        graph_neighbors_begin(g, start, &(pilha[topo].it));
        estado[start] = CINZA;

        while (topo >= 0 && !encontrou_ciclo) {
            size_t u = pilha[topo].v;
            size_t w;
            int tem_vizinho = graph_neighbors_next(&(pilha[topo].it), &w);

            if (tem_vizinho) {
                if (estado[w] == CINZA) {
                    /* Aresta de RETORNO encontrada! Ciclo detectado */
                    reportar_ciclo(pilha, topo, w);
                    encontrou_ciclo = 1;
                } else if (estado[w] == BRANCO) {
                    /* Aresta de ÁRVORE: avança na busca */
                    estado[w] = CINZA;
                    topo++;
                    pilha[topo].v = w;
                    graph_neighbors_begin(g, w, &(pilha[topo].it));
                }
                /* Arestas de AVANÇO ou CRUZADA (estado PRETO) são ignoradas na detecção */
            } else {
                /* Todos os vizinhos explorados: finaliza o vértice */
                estado[u] = PRETO;
                topo--;
            }
        }
    }

    mt_free(estado);
    mt_free(pilha);
    return encontrou_ciclo;
}
