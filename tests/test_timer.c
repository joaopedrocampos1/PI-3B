/*
 * Testes do cronômetro (src/timer.c), que mede o tempo do RF03.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_timer && ./bin/test_timer
 */
#include "timer.h"

#include <stdio.h>
#include <time.h>

static int falhas = 0;
static int verificacoes = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        verificacoes++;                                                      \
        if (!(cond)) {                                                       \
            falhas++;                                                        \
            fprintf(stderr, "  FALHOU %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                    \
    } while (0)

/* Ocupa a CPU por pelo menos `ms` milissegundos de tempo de processador,
 * medidos pelo clock() do C padrão: uma referência independente do timer. */
static void gastar_cpu(double ms)
{
    clock_t inicio = clock();
    volatile unsigned long long soma = 0;
    while ((double)(clock() - inicio) * 1000.0 / CLOCKS_PER_SEC < ms)
        for (int i = 0; i < 10000; i++)
            soma += (unsigned long long)i;
    /* O volatile já impede o compilador de apagar o laço, mas o gcc 16 ainda
     * acusa "set but not used" (erro com -Werror). Ler a variável resolve. */
    (void)soma;
}

/* O tempo medido nunca é negativo, nem logo depois de criar o timer. */
static void nunca_negativo(void)
{
    Timer *t = timer_criar();
    CHECK(t != NULL);
    CHECK(timer_parar_ms(t) >= 0.0);
    timer_iniciar(t);
    CHECK(timer_parar_ms(t) >= 0.0);
    timer_destruir(t);
}

/* Depois de 50 ms de CPU, o timer marca pelo menos uns 40 ms (o relógio de
 * parede não anda menos que o de CPU num processo de uma thread; a folga
 * cobre a granularidade do clock()). O teto alto só pega erro de unidade:
 * segundos no lugar de ms daria ~0,05; microssegundos, ~50.000. */
static void mede_em_milissegundos(void)
{
    Timer *t = timer_criar();
    timer_iniciar(t);
    gastar_cpu(50.0);
    double ms = timer_parar_ms(t);
    CHECK(ms >= 40.0);
    CHECK(ms < 10000.0);
    timer_destruir(t);
}

/* Parar não zera o início: duas leituras seguidas nunca diminuem
 * (relógio monotônico), e iniciar de novo recomeça a contagem. */
static void monotonico_e_reiniciavel(void)
{
    Timer *t = timer_criar();
    timer_iniciar(t);
    gastar_cpu(20.0);
    double a = timer_parar_ms(t);
    double b = timer_parar_ms(t);
    CHECK(b >= a);

    timer_iniciar(t);
    double c = timer_parar_ms(t);
    CHECK(c < a);
    timer_destruir(t);
}

/* Intervalos acima de um segundo: a conta precisa juntar a parte dos
 * segundos com a dos nanossegundos (ou dos ticks), não só a fração. É o caso
 * real dos algoritmos mais lentos, como a separação no grafo completo. */
static void intervalo_acima_de_um_segundo(void)
{
    Timer *t = timer_criar();
    timer_iniciar(t);
    gastar_cpu(1100.0);
    double ms = timer_parar_ms(t);
    CHECK(ms >= 1000.0);
    CHECK(ms < 60000.0);
    timer_destruir(t);
}

/* As funções aceitam NULL, que é o que timer_criar devolve sem memória. */
static void aceita_null(void)
{
    timer_iniciar(NULL);
    CHECK(timer_parar_ms(NULL) == 0.0);
    timer_destruir(NULL);
}

int main(void)
{
    nunca_negativo();
    mede_em_milissegundos();
    monotonico_e_reiniciavel();
    intervalo_acima_de_um_segundo();
    aceita_null();

    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
