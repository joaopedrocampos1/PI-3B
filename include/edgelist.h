#ifndef EDGELIST_H
#define EDGELIST_H

#include <stddef.h>

/*
 * Leitor de edge list: um arquivo de texto com uma aresta por linha, no
 * formato "origem destino".
 *
 * - Separadores aceitos: espaço, tab, vírgula e ponto e vírgula.
 * - Linhas vazias e linhas iniciadas por '#' são ignoradas.
 * - Cada linha de dados precisa ter exatamente dois inteiros não negativos;
 *   qualquer outra coisa é contada como malformada e descartada.
 * - Laços (origem == destino) e arestas repetidas são descartados.
 *   O grafo é tratado como direcionado: (A, B) e (B, A) são arestas distintas.
 *
 * Os IDs são devolvidos como estão no arquivo. O remapeamento para 0..V-1
 * é responsabilidade de outro módulo.
 */

typedef struct {
    unsigned long long origem;
    unsigned long long destino;
} Aresta;

typedef struct {
    Aresta *arestas;          /* ordenadas por (origem, destino) */
    size_t n;                 /* arestas mantidas */

    size_t malformadas;
    size_t primeira_malformada; /* número da linha (1 = primeira), 0 se não houver */
    size_t lacos;
    size_t duplicadas;
} EdgeList;

typedef enum {
    EDGELIST_OK = 0,
    EDGELIST_ERRO_ARQUIVO,    /* arquivo não pôde ser aberto ou lido */
    EDGELIST_ERRO_MEMORIA
} EdgeListStatus;

/* Lê o arquivo em `caminho` e preenche `saida`. Em caso de erro, `saida`
 * fica vazia e não precisa ser liberada. */
EdgeListStatus edgelist_ler(const char *caminho, EdgeList *saida);

void edgelist_liberar(EdgeList *el);

#endif
