#include "idmap.h"
#include "memtrack.h"

#include <stdlib.h>
#include <string.h>

#define CAP_INICIAL 16

/* splitmix64: espalha bits de IDs próximos por toda a tabela. Usar o próprio
 * ID como hash concentraria IDs sequenciais em slots vizinhos. */
static unsigned long long espalhar(unsigned long long x)
{
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

/* Slot onde `id` está ou deveria estar. */
static size_t sondar(const IdMap *m, unsigned long long id)
{
    size_t mascara = m->cap_tabela - 1;
    size_t s = (size_t)(espalhar(id) & mascara);
    while (m->tabela[s] != 0 && m->originais[m->tabela[s] - 1] != id)
        s = (s + 1) & mascara;
    return s;
}

/* Troca a tabela por uma com `nova_cap` slots e reposiciona todos os IDs. */
static int redimensionar_tabela(IdMap *m, size_t nova_cap)
{
    size_t *nova = mt_calloc(nova_cap, sizeof *nova);
    if (!nova)
        return 0;

    mt_free(m->tabela);
    m->tabela = nova;
    m->cap_tabela = nova_cap;
    for (size_t k = 0; k < m->n; k++)
        m->tabela[sondar(m, m->originais[k])] = k + 1;
    return 1;
}

/* Garante espaço para mais um ID: o mapa inverso tem posição livre e a
 * tabela fica no máximo metade ocupada, o que mantém as sondagens curtas. */
static int reservar_um(IdMap *m)
{
    if (m->n == m->cap_originais) {
        size_t nova = m->cap_originais ? m->cap_originais * 2 : CAP_INICIAL;
        unsigned long long *p = mt_realloc(m->originais, nova * sizeof *p);
        if (!p)
            return 0;
        m->originais = p;
        m->cap_originais = nova;
    }

    if ((m->n + 1) * 2 > m->cap_tabela)
        return redimensionar_tabela(m, m->cap_tabela ? m->cap_tabela * 2 : CAP_INICIAL);
    return 1;
}

void idmap_iniciar(IdMap *m)
{
    memset(m, 0, sizeof *m);
}

int idmap_inserir(IdMap *m, unsigned long long id, size_t *indice)
{
    if (idmap_buscar(m, id, indice))
        return 1;

    if (!reservar_um(m))
        return 0;

    size_t s = sondar(m, id);   /* a tabela pode ter mudado de tamanho */
    m->originais[m->n] = id;
    m->tabela[s] = ++m->n;
    *indice = m->n - 1;
    return 1;
}

int idmap_buscar(const IdMap *m, unsigned long long id, size_t *indice)
{
    if (m->n == 0)   /* tabela ainda não alocada */
        return 0;

    size_t s = sondar(m, id);
    if (m->tabela[s] == 0)
        return 0;

    *indice = m->tabela[s] - 1;
    return 1;
}

unsigned long long idmap_original(const IdMap *m, size_t indice)
{
    return m->originais[indice];
}

void idmap_liberar(IdMap *m)
{
    mt_free(m->tabela);
    mt_free(m->originais);
    idmap_iniciar(m);
}
