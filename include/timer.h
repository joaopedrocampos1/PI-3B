#ifndef TIMER_H
#define TIMER_H

typedef struct Timer Timer;

Timer* timer_criar(void);
void   timer_iniciar(Timer *t);
double timer_parar_ms(Timer *t);
void   timer_destruir(Timer *t);

#endif /* TIMER_H */
