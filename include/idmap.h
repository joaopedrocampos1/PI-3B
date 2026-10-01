#ifndef IDMAP_H
#define IDMAP_H

#include <stddef.h>

/*
 * Mapeamento de IDs: associa cada ID original do dataset (esparso, até
 * 64 bits) a um índice denso 0..V-1, que é o que as estruturas de grafo
 * usam como posição. Guarda também o mapa inverso, índice -> ID original,
 * para exibir resultados com os IDs reais.
 *
 * Os índices são distribuídos na ordem em que os IDs aparecem: o primeiro
 * ID inserido recebe 0, o seguinte 1, e assim por diante.
 *
 * Implementação: tabela hash com endereçamento aberto e sondagem linear.
 */

typedef struct {
    unsigned long long *originais; /* mapa inverso: índice -> ID original */
    size_t n;                      /* quantidade de IDs distintos mapeados */
    size_t cap_originais;

    size_t *tabela;                /* slots: 0 = vazio, k = índice k - 1 */
    size_t cap_tabela;             /* sempre potência de 2 */
} IdMap;

/* Deixa o mapa vazio e pronto para uso. Não aloca memória. */
void idmap_iniciar(IdMap *m);

/* Devolve em *indice o índice de `id`, criando um novo se o ID ainda não
 * estiver mapeado. Retorna 1 em caso de sucesso e 0 se faltar memória. */
int idmap_inserir(IdMap *m, unsigned long long id, size_t *indice);

/* Procura `id` sem inseri-lo. Retorna 1 e preenche *indice se encontrado,
 * ou 0 se o ID não estiver mapeado. */
int idmap_buscar(const IdMap *m, unsigned long long id, size_t *indice);

/* ID original do vértice `indice`. Exige indice < m->n. */
unsigned long long idmap_original(const IdMap *m, size_t indice);

void idmap_liberar(IdMap *m);

#endif
