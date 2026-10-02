#ifndef DOT_H
#define DOT_H

#include "graph.h"
#include "idmap.h"

#include <stddef.h>

/*
 * Exportação em formato Graphviz (.dot), para visualizar grafos pequenos
 * (amostras, casos de teste) com o resultado de um algoritmo. Renderizar:
 *   dot -Tpng arquivo.dot -o arquivo.png
 *
 * Na visão direcionada sai um "digraph", com as arestas u -> v. Nas visões
 * sem sentido sai um "graph", com cada aresta {u, v} uma vez só. Cada vértice
 * aparece com o seu ID original.
 *
 * Com mais de algumas centenas de vértices, a imagem vira uma mancha: a
 * exportação funciona com qualquer tamanho, mas só é legível em grafos
 * pequenos.
 */

typedef struct {
    size_t u, v;                     /* índices das pontas */
} DotAresta;

/* O que destacar no desenho. Todos os campos podem ser NULL / 0. */
typedef struct {
    const char *const *cor;          /* cor de preenchimento de cada vértice (V posições),
                                        como "#2a78d6"; NULL num vértice = sem cor */
    const unsigned char *destaque;   /* 1 para os vértices desenhados com borda grossa */
    const DotAresta *arestas;        /* arestas desenhadas em destaque; nas visões sem
                                        sentido, a ordem das pontas não importa */
    size_t num_arestas;
} DotEstilo;

#define DOT_COR_DESTAQUE "#e34948"

typedef enum {
    DOT_OK = 0,
    DOT_ERRO_ARQUIVO,                /* não abriu ou não gravou (a pasta existe?) */
    DOT_ERRO_MEMORIA
} DotStatus;

/* Grava `g` em `caminho`. `ids` traduz índices para os IDs originais; `estilo`
 * e `titulo` podem ser NULL. */
DotStatus dot_gravar(const char *caminho, const Graph *g, const IdMap *ids,
                     const DotEstilo *estilo, const char *titulo);

#endif
