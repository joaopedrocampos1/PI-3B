#include "../include/graph.h"
#include "memtrack.h"
#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Estrutura para a pilha explícita do Tarjan para evitar Stack Overflow */
typedef struct {
    size_t u;
    size_t pai;
    size_t filhos;
    GraphIter it;
    int estado; /* 0: primeira visita, 1: processando vizinhos */
} TarjanFrame;

/* Função para encontrar e exibir os vértices de articulação (pontes de informação) */
void graph_analisar_articulacoes(const Graph *g, size_t top_n) {
    size_t n = graph_num_vertices(g);
    if (n == 0) return;

    size_t *num = mt_calloc(n, sizeof(size_t));
    size_t *low = mt_calloc(n, sizeof(size_t));
    int *eh_articulacao = mt_calloc(n, sizeof(int));
    size_t *impacto = mt_calloc(n, sizeof(size_t)); // Guarda o grau do vértice como métrica de impacto
    TarjanFrame *pilha = mt_malloc(n * sizeof(TarjanFrame));

    if (!num || !low || !eh_articulacao || !pilha || !impacto) {
        if (num) mt_free(num);
        if (low) mt_free(low);
        if (eh_articulacao) mt_free(eh_articulacao);
        if (impacto) mt_free(impacto);
        if (pilha) mt_free(pilha);
        return;
    }

    size_t tempo = 0;
    int topo = -1;

    for (size_t i = 0; i < n; i++) {
        if (num[i] != 0) continue;

        // Inicia uma nova árvore do DFS para componentes desconexos
        topo++;
        pilha[topo].u = i;
        pilha[topo].pai = n; // Valor sentinela significando "sem pai"
        pilha[topo].filhos = 0;
        pilha[topo].estado = 0;
        graph_neighbors_begin(g, i, &(pilha[topo].it));

        while (topo >= 0) {
            size_t u = pilha[topo].u;
            size_t pai = pilha[topo].pai;

            if (pilha[topo].estado == 0) {
                tempo++;
                num[u] = tempo;
                low[u] = tempo;
                pilha[topo].estado = 1;
            }

            size_t v;
            int tem_vizinho = graph_neighbors_next(&(pilha[topo].it), &v);

            if (tem_vizinho) {
                if (v == pai) continue; // Ignora a aresta que veio do pai direto

                if (num[v] != 0) {
                    // Aresta de retorno: atualiza o low de 'u'
                    low[u] = MIN(low[u], num[v]);
                } else {
                    // Aresta de árvore: incrementa os filhos e empilha o vizinho 'v'
                    pilha[topo].filhos++;
                    
                    topo++;
                    pilha[topo].u = v;
                    pilha[topo].pai = u;
                    pilha[topo].filhos = 0;
                    pilha[topo].estado = 0;
                    graph_neighbors_begin(g, v, &(pilha[topo].it));
                }
            } else {
                // Todos os vizinhos processados: faz o backtracking para o pai
                if (topo > 0) {
                    size_t p = pilha[topo - 1].u;
                    low[p] = MIN(low[p], low[u]);

                    // Condição de articulação para vértices que não são a raiz da árvore DFS
                    if (pilha[topo - 1].pai != n && low[u] >= num[p]) {
                        eh_articulacao[p] = 1;
                    }
                } else {
                    // Condição de articulação para a raiz da árvore DFS
                    if (pilha[topo].filhos > 1) {
                        eh_articulacao[u] = 1;
                    }
                }
                topo--;
            }
        }
    }

    // Calcula o impacto com base no grau (quanto maior o grau de uma articulação, maior a fragmentação)
    size_t total_articulacoes = 0;
    for (size_t i = 0; i < n; i++) {
        if (eh_articulacao[i]) {
            impacto[i] = graph_degree(g, i);
            total_articulacoes++;
        }
    }

    printf("\n=== RESULTADO: VERTICES DE ARTICULACAO (TARJAN) ===\n");
    printf("Total de usuarios-ponte detectados: %zu\n", total_articulacoes);
    printf("Exibindo o Top-%zu por impacto estimado de fragmentacao:\n", top_n < total_articulacoes ? top_n : total_articulacoes);

    // Ordenação simples (Bubble Sort) para listar os Top-N por impacto
    for (size_t i = 0; i < top_n && i < n; i++) {
        size_t max_idx = i;
        for (size_t j = i + 1; j < n; j++) {
            if (impacto[j] > impacto[max_idx]) {
                max_idx = j;
            }
        }
        if (impacto[max_idx] == 0) break; // Acabaram os vértices de articulação

        printf("Top %zu: Usuario ID Mapeado %zu (Conexoes afetadas: %zu)\n", i + 1, max_idx, impacto[max_idx]);
        
        // Zera o maior para não pegar repetido na ordenação
        impacto[max_idx] = 0; 
    }
    printf("==================================================\n\n");

    mt_free(num);
    mt_free(low);
    mt_free(eh_articulacao);
    mt_free(impacto);
    mt_free(pilha);
}
