#ifndef MEMTRACK_H
#define MEMTRACK_H

/*
 * Medição de memória (#24, RF03).
 *
 * REGRA DO PROJETO: todo o código usa mt_malloc / mt_calloc / mt_realloc /
 * mt_free no lugar de malloc / calloc / realloc / free. Sem isso, a memória
 * da estrutura não é contabilizada e a comparação lista x matriz fica errada.
 *
 * Duas medidas complementares:
 *  - mt_*_bytes: contador autoral, exato, só dos bytes pedidos pelo projeto.
 *    Pode ser zerado entre algoritmos (mt_reset_peak), então mede cada um.
 *  - mt_peak_rss_kb: pico de memória residente do processo inteiro, segundo
 *    o sistema operacional. Não pode ser zerado: vale para a execução toda.
 */

#include <stddef.h>

void  *mt_malloc(size_t size);
void  *mt_calloc(size_t n, size_t size);
void  *mt_realloc(void *p, size_t size);
void   mt_free(void *p);              /* aceita NULL */

size_t mt_current_bytes(void);        /* bytes alocados agora              */
size_t mt_peak_bytes(void);           /* maior valor desde o último reset  */
void   mt_reset_peak(void);           /* pico passa a ser o valor atual    */

long   mt_peak_rss_kb(void);          /* pico residente do processo, -1 se falhar */

#endif
