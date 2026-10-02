#include "cycles.h"
#include "memtrack.h"

#include <string.h>

/* Estados dos vértices para a detecção de ciclos */
#define BRANCO 0 /* Não visitado */
#define CINZA  1 /* Na pilha de exploração (ativo) */
#define PRETO  2 /* Totalmente explorado */

typedef struct {
    size_t v;
    GraphIter it;
} CycleFrame;

/* Guarda o ciclo fechado pela aresta de retorno topo -> destino: os vértices
 * da pilha desde `destino` até o topo. */
static int guardar_ciclo(CiclosResultado *r, const CycleFrame *pilha, size_t topo, size_t destino)
{
    // Encontra onde o ciclo começa na pilha
    size_t i = 0;
    while (i <= topo && pilha[i].v != destino)
        i++;

    r->tam_ciclo = topo - i + 1;
    r->ciclo = mt_malloc(r->tam_ciclo * sizeof *r->ciclo);
    if (!r->ciclo)
        return 0;
    for (size_t j = i; j <= topo; j++)
        r->ciclo[j - i] = pilha[j].v;
    return 1;
}

/* Detecção de ciclos com pilha explícita e classificação de todas as arestas */
CiclosStatus ciclos_executar(const Graph *g, CiclosResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    size_t alocar = n ? n : 1;

    unsigned char *estado = mt_calloc(alocar, 1);
    size_t *descoberta = mt_malloc(alocar * sizeof *descoberta);   /* ordem de descoberta */
    CycleFrame *pilha = mt_malloc(alocar * sizeof *pilha);
    int ok = estado && descoberta && pilha;
    size_t tempo = 0;

    // Percorre todos os vértices para cobrir grafos desconexos
    for (size_t start = 0; ok && start < n; start++) {
        if (estado[start] != BRANCO)
            continue;

        size_t topo = 0;
        pilha[topo].v = start;
        graph_neighbors_begin(g, start, &pilha[topo].it);
        estado[start] = CINZA;
        descoberta[start] = tempo++;

        for (;;) {
            size_t u = pilha[topo].v;
            size_t w;

            if (graph_neighbors_next(&pilha[topo].it, &w)) {
                if (estado[w] == BRANCO) {
                    /* Aresta de ÁRVORE: avança na busca */
                    r->arvore++;
                    estado[w] = CINZA;
                    descoberta[w] = tempo++;
                    topo++;
                    pilha[topo].v = w;
                    graph_neighbors_begin(g, w, &pilha[topo].it);
                } else if (estado[w] == CINZA) {
                    /* Aresta de RETORNO: fecha um ciclo; o primeiro vira o exemplo */
                    r->retorno++;
                    if (!r->tem_ciclo) {
                        r->tem_ciclo = 1;
                        if (!guardar_ciclo(r, pilha, topo, w)) {
                            ok = 0;
                            break;
                        }
                    }
                } else if (descoberta[u] < descoberta[w]) {
                    r->avanco++;    /* AVANÇO: w é descendente de u já finalizado */
                } else {
                    r->cruzada++;   /* CRUZADA: w está em outro ramo ou em outra árvore */
                }
            } else {
                /* Todos os vizinhos explorados: finaliza o vértice */
                estado[u] = PRETO;
                if (topo == 0)
                    break;
                topo--;
            }
        }
    }

    mt_free(estado);
    mt_free(descoberta);
    mt_free(pilha);
    if (!ok) {
        ciclos_liberar(r);
        return CICLOS_ERRO_MEMORIA;
    }
    return CICLOS_OK;
}

void ciclos_liberar(CiclosResultado *r)
{
    mt_free(r->ciclo);
    memset(r, 0, sizeof *r);
}
