#include "clique.h"
#include "memtrack.h"
#include "timer.h"

#include <stdint.h>
#include <string.h>

/*
 * Um nível da busca: os candidatos que ainda podem entrar na clique atual,
 * ordenados por cor, e a cor de cada um (1, 2, ...). A cor de p[j] é um
 * limitante para a clique que cabe em p[0..j]: os vértices de uma mesma cor
 * não são vizinhos, então no máximo um de cada cor entra na clique.
 *
 * Os candidatos são tentados do fim para o começo, isto é, da maior cor para
 * a menor. Depois de tentar p[i], ele sai do conjunto: os ramos seguintes só
 * olham para p[0..i-1].
 */
typedef struct {
    size_t *p;      /* candidatos; no nível 0, aponta para o vetor da raiz */
    size_t *col;    /* cor de cada candidato, não decrescente */
    size_t i;       /* candidatos ainda não tentados: p[0..i-1] */
} Nivel;

/* Verifica o relógio a cada 128 nós, para não pagar uma leitura por nó. */
#define CHECAR_TEMPO_A_CADA 128ULL

/* Ordena `v` (tamanho m) pela chave chave[v[k]], de forma estável.
 * Por inserção: m é o grau de um vértice, pequeno nos grafos da bateria. */
static void ordenar_por_chave(size_t *v, size_t m, const size_t *chave)
{
    for (size_t a = 1; a < m; a++) {
        size_t x = v[a], b = a;
        while (b > 0 && chave[v[b - 1]] > chave[x]) {
            v[b] = v[b - 1];
            b--;
        }
        v[b] = x;
    }
}

static void ordenar_crescente(size_t *v, size_t m)
{
    for (size_t a = 1; a < m; a++) {
        size_t x = v[a], b = a;
        while (b > 0 && v[b - 1] > x) {
            v[b] = v[b - 1];
            b--;
        }
        v[b] = x;
    }
}

/*
 * Coloração gulosa dos candidatos q[0..m-1] de um nível interno: cada um
 * recebe a menor cor que nenhum vizinho anterior tem. Saem em p, ordenados por
 * cor de forma estável, e em col, a cor de cada um (começando em 1).
 *
 * `cls` e `usado` são áreas de trabalho com m e m + 1 posições.
 */
static void colorir(const Graph *g, const size_t *q, size_t m, size_t *p, size_t *col,
                    size_t *cls, unsigned char *usado)
{
    size_t ncores = 0;
    for (size_t j = 0; j < m; j++) {
        memset(usado, 0, ncores + 1);
        for (size_t t = 0; t < j; t++)
            if (graph_has_edge(g, q[j], q[t]))
                usado[cls[t]] = 1;
        size_t c = 0;
        while (usado[c])
            c++;
        cls[j] = c;
        if (c + 1 > ncores)
            ncores = c + 1;
    }

    size_t k = 0;
    for (size_t c = 0; c < ncores; c++)
        for (size_t j = 0; j < m; j++)
            if (cls[j] == c) {
                p[k] = q[j];
                col[k] = c + 1;
                k++;
            }
}

/*
 * Nível da raiz: todos os vértices. Aqui o grafo inteiro é colorido, então a
 * coloração usa o percurso de vizinhos, O(V + E), e não pares de vértices.
 *
 * Os vértices são coloridos em ordem decrescente de grau (empate pelo menor
 * índice): os de grau alto ficam com as cores baixas e são tentados por
 * último, quando a melhor clique já é grande e poda mais.
 *
 * As duas ordenações (por grau e depois por cor) são por contagem, estáveis,
 * para a raiz custar O(V + E) também no grafo inteiro.
 *
 * pos[v] guarda a posição de v em p, para montar os candidatos do nível 1.
 * `cont` é área de trabalho com n + 1 posições.
 */
static void preparar_raiz(const Graph *g, size_t n, size_t grau_max, size_t *p, size_t *col,
                          size_t *pos, size_t *ordem, size_t *cor, size_t *marca, size_t *cont)
{
    /* ordem: grau decrescente, empate pelo menor índice */
    memset(cont, 0, (grau_max + 2) * sizeof *cont);
    for (size_t v = 0; v < n; v++)
        cont[grau_max - graph_degree(g, v) + 1]++;
    for (size_t d = 1; d <= grau_max + 1; d++)
        cont[d] += cont[d - 1];
    for (size_t v = 0; v < n; v++)
        ordem[cont[grau_max - graph_degree(g, v)]++] = v;

    const size_t SEM_COR = SIZE_MAX;
    size_t ncores = 0;
    for (size_t v = 0; v < n; v++) {
        cor[v] = SEM_COR;
        marca[v] = SIZE_MAX;
    }
    for (size_t k = 0; k < n; k++) {
        size_t v = ordem[k];
        GraphIter it;
        size_t w;
        graph_neighbors_begin(g, v, &it);
        while (graph_neighbors_next(&it, &w))
            if (cor[w] != SEM_COR)
                marca[cor[w]] = v;   /* cor[w] está ocupada para v */
        size_t c = 0;
        while (marca[c] == v)
            c++;
        cor[v] = c;
        if (c + 1 > ncores)
            ncores = c + 1;
    }

    /* p: por cor crescente, mantendo a ordem de grau dentro de cada cor */
    memset(cont, 0, (ncores + 1) * sizeof *cont);
    for (size_t v = 0; v < n; v++)
        cont[cor[v] + 1]++;
    for (size_t c = 1; c <= ncores; c++)
        cont[c] += cont[c - 1];
    for (size_t k = 0; k < n; k++) {
        size_t v = ordem[k], j = cont[cor[v]]++;
        p[j] = v;
        col[j] = cor[v] + 1;
        pos[v] = j;
    }
}

CliqueStatus clique_exata(const Graph *g, double limite_ms, CliqueResultado *r)
{
    memset(r, 0, sizeof *r);
    size_t n = graph_num_vertices(g);
    r->n = n;
    r->exato = 1;
    if (n == 0)
        return CLIQUE_OK;

    size_t grau_max = 0;
    for (size_t v = 0; v < n; v++) {
        size_t d = graph_degree(g, v);
        if (d > grau_max)
            grau_max = d;
    }
    /* A clique máxima tem no máximo grau_max + 1 vértices, e a pilha tem um
     * nível a mais que a clique. Nenhum nível interno tem mais candidatos que
     * o grau de um vértice. */
    size_t prof_max = grau_max + 2;
    size_t g1 = grau_max + 1;

    size_t *raiz_p = mt_malloc(n * sizeof *raiz_p);
    size_t *raiz_col = mt_malloc(n * sizeof *raiz_col);
    size_t *pos = mt_malloc(n * sizeof *pos);
    size_t *ordem = mt_malloc(n * sizeof *ordem);
    size_t *cor = mt_malloc(n * sizeof *cor);
    size_t *marca = mt_malloc(n * sizeof *marca);
    size_t *cont = mt_malloc((n + 1) * sizeof *cont);
    Nivel *pilha = mt_malloc(prof_max * sizeof *pilha);
    size_t *atual = mt_malloc(g1 * sizeof *atual);
    size_t *melhor = mt_malloc(g1 * sizeof *melhor);
    size_t *tmp = mt_malloc(g1 * sizeof *tmp);
    size_t *cls = mt_malloc(g1 * sizeof *cls);
    unsigned char *usado = mt_malloc(g1 + 1);
    Timer *t = limite_ms > 0 ? timer_criar() : NULL;

    int ok = raiz_p && raiz_col && pos && ordem && cor && marca && cont && pilha && atual && melhor &&
             tmp && cls && usado && (limite_ms <= 0 || t);
    size_t prof = 0, tam_melhor = 0;
    unsigned long long nos = 0;
    int parou = 0;

    if (ok) {
        preparar_raiz(g, n, grau_max, raiz_p, raiz_col, pos, ordem, cor, marca, cont);
        pilha[0].p = raiz_p;
        pilha[0].col = raiz_col;
        pilha[0].i = n;
        prof = 1;
    }

    while (ok && prof > 0) {
        Nivel *nv = &pilha[prof - 1];
        size_t tam_atual = prof - 1;

        /* Sem candidatos, ou nem pegando um de cada cor a clique atual
         * supera a melhor: as cores só diminuem daqui para trás, então o
         * nível inteiro é podado. */
        if (nv->i == 0 || tam_atual + nv->col[nv->i - 1] <= tam_melhor) {
            if (prof > 1)
                mt_free(nv->p);   /* p e col vêm do mesmo bloco */
            prof--;
            continue;
        }

        size_t i = --nv->i;
        size_t v = nv->p[i];
        atual[tam_atual] = v;
        nos++;
        if (t && nos % CHECAR_TEMPO_A_CADA == 0 && timer_parar_ms(t) > limite_ms) {
            /* atual[0..tam_atual] também é uma clique: guarda se for maior */
            if (tam_atual + 1 > tam_melhor) {
                tam_melhor = tam_atual + 1;
                memcpy(melhor, atual, tam_melhor * sizeof *melhor);
            }
            parou = 1;
            break;
        }

        /* Candidatos do próximo nível: os de p[0..i-1] vizinhos de v, na
         * mesma ordem. */
        size_t m = 0;
        if (prof == 1) {
            GraphIter it;
            size_t w;
            graph_neighbors_begin(g, v, &it);
            while (graph_neighbors_next(&it, &w))
                if (pos[w] < i)
                    tmp[m++] = w;
            ordenar_por_chave(tmp, m, pos);
        } else {
            for (size_t j = 0; j < i; j++)
                if (graph_has_edge(g, v, nv->p[j]))
                    tmp[m++] = nv->p[j];
        }

        if (m == 0) {
            if (tam_atual + 1 > tam_melhor) {
                tam_melhor = tam_atual + 1;
                memcpy(melhor, atual, tam_melhor * sizeof *melhor);
            }
            continue;
        }
        if (tam_atual + 1 + m <= tam_melhor)
            continue;   /* nem todos os candidatos juntos bastariam */

        size_t *bloco = mt_malloc(2 * m * sizeof *bloco);
        if (!bloco) {
            ok = 0;
            break;
        }
        Nivel *novo = &pilha[prof++];
        novo->p = bloco;
        novo->col = bloco + m;
        novo->i = m;
        colorir(g, tmp, m, novo->p, novo->col, cls, usado);
    }

    for (size_t k = 1; k < prof; k++)
        mt_free(pilha[k].p);

    if (ok) {
        r->clique = mt_malloc(tam_melhor * sizeof *r->clique);
        ok = r->clique != NULL;
    }
    if (ok) {
        memcpy(r->clique, melhor, tam_melhor * sizeof *r->clique);
        ordenar_crescente(r->clique, tam_melhor);
        r->tam = tam_melhor;
        r->nos = nos;
        r->exato = !parou;
    }

    timer_destruir(t);
    mt_free(usado);
    mt_free(cls);
    mt_free(tmp);
    mt_free(melhor);
    mt_free(atual);
    mt_free(pilha);
    mt_free(cont);
    mt_free(marca);
    mt_free(cor);
    mt_free(ordem);
    mt_free(pos);
    mt_free(raiz_col);
    mt_free(raiz_p);

    if (!ok) {
        mt_free(r->clique);
        memset(r, 0, sizeof *r);
        return CLIQUE_ERRO_MEMORIA;
    }
    return CLIQUE_OK;
}

void clique_liberar(CliqueResultado *r)
{
    mt_free(r->clique);
    memset(r, 0, sizeof *r);
}
