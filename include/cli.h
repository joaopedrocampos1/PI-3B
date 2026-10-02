#ifndef CLI_H
#define CLI_H

#include <stddef.h>
#include <stdio.h>

/*
 * Leitura dos argumentos de linha de comando.
 *
 * Toda opção aceita as formas "--opcao valor" e "--opcao=valor". Se uma
 * opção se repete, vale a última.
 */

typedef enum {
    ESTRUTURA_LISTA,
    ESTRUTURA_MATRIZ
} Estrutura;

typedef enum {
    ALGO_NENHUM,
    ALGO_BFS,
    ALGO_DFS,
    ALGO_COMPONENTES,
    ALGO_CICLOS,
    ALGO_BIPARTIDO,
    ALGO_ARTICULACAO,
    ALGO_SEPARACAO      /* graus de separação perfil comum -> influenciador (#17) */
} Algoritmo;

typedef struct {
    const char *entrada;              /* --input (obrigatório) */
    Estrutura estrutura;              /* --struct lista|matriz; padrão: lista */
    Algoritmo algoritmo;              /* --algo; padrão: nenhum */
    int tem_origem;                   /* --source foi informado? */
    unsigned long long origem;        /* --source: ID original do vértice */
    const char *saida;                /* --output; NULL se ausente */
    const char *pasta_amostras;       /* --amostrar: pasta onde gravar as amostras */
    unsigned long long semente_rng;   /* --semente-rng; padrão: CLI_SEMENTE_RNG_PADRAO */
    unsigned execucao;                /* --execucao: número desta execução no log; padrão: 1 */
    const char *log;                  /* --log: CSV das medições; padrão: LOG_CAMINHO_PADRAO */
} Opcoes;

#define CLI_SEMENTE_RNG_PADRAO 2026ULL

typedef enum {
    CLI_OK,
    CLI_AJUDA,   /* pediu --help */
    CLI_ERRO     /* argumentos inválidos; a mensagem vai para `erro` */
} CliStatus;

/* Preenche `op` a partir de argv. Em CLI_ERRO, escreve em `erro` (até
 * `tam_erro` bytes) uma mensagem para o usuário. */
CliStatus cli_ler(int argc, char **argv, Opcoes *op, char *erro, size_t tam_erro);

/* Texto de uso, com todas as opções. */
void cli_uso(FILE *f, const char *programa);

/* Nome do algoritmo como escrito na linha de comando ("bfs", ...). */
const char *cli_nome_algoritmo(Algoritmo a);

#endif
