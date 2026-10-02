#ifndef SEPARACAO_H
#define SEPARACAO_H

#include "graph.h"

#include <stddef.h>

/*
 * Graus de separação (#17).
 *
 * Pergunta do estudo de caso: qual o grau de separação médio entre perfis
 * comuns e grandes influenciadores da rede?
 *
 * Direção. Na visão direcionada, "A segue B" é a aresta A -> B, e a
 * informação anda no sentido contrário: uma publicação de B aparece para A.
 * Um BFS a partir de um perfil comum P, seguindo as arestas, mede portanto
 * quantos repasses a publicação de cada vértice precisa para chegar até P.
 * A distância de P até um influenciador I é o número de repasses para o
 * conteúdo de I alcançar P.
 *
 * Influenciador. Os vértices com mais seguidores (grau de entrada na visão
 * direcionada). O critério é o percentil: entram os `fracao` x V vértices de
 * maior grau de entrada, arredondando para cima, e também todos os empatados
 * com o último deles, para que o resultado não dependa da ordem dos índices.
 *
 * Perfis comuns. Sorteio uniforme, sem reposição e reprodutível pela
 * semente, entre os vértices que não são influenciadores.
 */

#define SEPARACAO_FRACAO_PADRAO   0.01   /* 1% com mais seguidores */
#define SEPARACAO_PERFIS_PADRAO   1000

typedef struct {
    size_t vertice;
    size_t alcancados;            /* incluindo ele mesmo */
    size_t excentricidade;        /* maior distância até um alcançado */
    double dist_media;            /* média até os alcançados; 0 se nenhum */
    size_t dist_influenciador;    /* até o influenciador mais próximo; SIZE_MAX se nenhum */
    double dist_media_influenciadores;  /* média até os influenciadores alcançados */
    size_t influenciadores_alcancados;
} SeparacaoPerfil;

typedef struct {
    /* influenciadores */
    size_t num_influenciadores;
    size_t grau_entrada_corte;    /* menor grau de entrada entre eles */
    unsigned char *eh_influenciador;

    /* perfis comuns analisados */
    size_t num_perfis;
    SeparacaoPerfil *perfis;

    /* histogramas por número de saltos, com hist_tam posições (0..hist_tam-1) */
    size_t hist_tam;
    size_t *hist_rede;            /* pares (perfil, vértice alcançado), sem o próprio perfil */
    size_t *hist_influenciador;   /* perfis por distância até o influenciador mais próximo */

    /* agregados */
    double dist_media_rede;            /* média sobre todos os pares de hist_rede */
    double dist_media_influenciador;   /* média de dist_influenciador, entre os que alcançam algum */
    double dist_media_conjunto;        /* média das distâncias até todos os influenciadores
                                          alcançados, sobre todos os pares */
    size_t perfis_sem_influenciador;   /* perfis que não alcançam nenhum */
    size_t diametro_estimado;          /* maior excentricidade entre os perfis: limite
                                          inferior do diâmetro (pares alcançáveis) */
} SeparacaoResultado;

typedef enum {
    SEPARACAO_OK = 0,
    SEPARACAO_ERRO_MEMORIA,
    SEPARACAO_ERRO_PARAMETRO   /* fração fora de (0, 1], grafo vazio ou sem perfis comuns */
} SeparacaoStatus;

/* Grau de entrada de cada vértice, em `grau` (V posições). */
void separacao_grau_entrada(const Graph *g, size_t *grau);

/* Executa a análise completa: define os influenciadores, sorteia até
 * `num_perfis` perfis comuns com `semente` e roda um BFS a partir de cada um.
 * Em erro, `r` fica zerado. */
SeparacaoStatus separacao_analisar(const Graph *g, double fracao, size_t num_perfis,
                                   unsigned long long semente, SeparacaoResultado *r);

/* Diâmetro e distância média exatos, por BFS a partir de todos os vértices,
 * considerando só os pares alcançáveis. O(V · (V + E)): para os subgrafos
 * amostrados, não para o grafo completo. */
SeparacaoStatus separacao_exato(const Graph *g, size_t *diametro, double *dist_media);

void separacao_liberar(SeparacaoResultado *r);   /* aceita resultado zerado */

#endif
