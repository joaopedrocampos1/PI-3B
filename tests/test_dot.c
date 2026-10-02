/*
 * Testes da exportação Graphviz (src/dot.c), nas duas representações.
 *
 * Compilar e rodar a partir da raiz do repositório:
 *   make bin/test_dot && ./bin/test_dot
 */
#include "dot.h"
#include "edgelist.h"
#include "memtrack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define ENTRADA "bin/fixture_dot.txt"
#define SAIDA "bin/teste.dot"

typedef struct {
    EdgeList el;
    IdMap ids;
    Graph *g;
} Carregado;

/* Monta o grafo a partir de uma edge list, pelo caminho do programa real,
 * para que os IDs originais sejam diferentes dos índices. */
static Carregado carregar(const char *arestas, GraphRep rep, GraphView view)
{
    Carregado c;
    FILE *f = fopen(ENTRADA, "w");
    if (!f) {
        perror(ENTRADA);
        exit(2);
    }
    fputs(arestas, f);
    fclose(f);
    if (edgelist_ler(ENTRADA, &c.el) != EDGELIST_OK) {
        fprintf(stderr, "fixture ilegível\n");
        exit(2);
    }
    idmap_iniciar(&c.ids);
    if (graph_build(&c.el, &c.ids, rep, view, &c.g) != GRAPH_OK) {
        fprintf(stderr, "sem memória\n");
        exit(2);
    }
    return c;
}

static void descarregar(Carregado *c)
{
    graph_destroy(c->g);
    idmap_liberar(&c->ids);
    edgelist_liberar(&c->el);
}

static size_t indice(const Carregado *c, unsigned long long id)
{
    size_t i;
    if (!idmap_buscar(&c->ids, id, &i)) {
        fprintf(stderr, "ID %llu ausente\n", id);
        exit(2);
    }
    return i;
}

/* Conteúdo do .dot gerado; o chamador libera. */
static char *ler_saida(void)
{
    FILE *f = fopen(SAIDA, "r");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    rewind(f);
    char *s = malloc((size_t)tam + 1);
    size_t lidos = fread(s, 1, (size_t)tam, f);
    s[lidos] = '\0';
    fclose(f);
    return s;
}

static size_t contar(const char *texto, const char *trecho)
{
    size_t n = 0;
    for (const char *p = texto; (p = strstr(p, trecho)) != NULL; p++)
        n++;
    return n;
}

/* A linha da aresta "a" <op> "b" existe, e contém (ou não) o atributo. */
static int linha_de_aresta(const char *texto, const char *a, const char *op, const char *b,
                           const char *atributo)
{
    char alvo[96];
    snprintf(alvo, sizeof alvo, "\"%s\" %s \"%s\"", a, op, b);
    const char *p = strstr(texto, alvo);
    if (!p)
        return 0;
    const char *fim = strchr(p, '\n');
    size_t tam = fim ? (size_t)(fim - p) : strlen(p);
    char linha[256];
    snprintf(linha, sizeof linha, "%.*s", (int)tam, p);
    return atributo == NULL || strstr(linha, atributo) != NULL;
}

/* Direcionada: digraph, uma linha "u" -> "v" por aresta, com os IDs originais. */
static void direcionada(GraphRep rep)
{
    Carregado c = carregar("500 723\n723 81\n81 500\n500 81\n", rep, GRAPH_DIRECTED);
    CHECK(dot_gravar(SAIDA, c.g, &c.ids, NULL, NULL) == DOT_OK);
    char *s = ler_saida();
    CHECK(s && strncmp(s, "digraph ", 8) == 0);
    CHECK(s && contar(s, " -> ") == 4 && contar(s, " -- ") == 0);
    CHECK(s && linha_de_aresta(s, "500", "->", "723", NULL));
    CHECK(s && linha_de_aresta(s, "81", "->", "500", NULL));
    CHECK(s && linha_de_aresta(s, "500", "->", "81", NULL));   /* recíprocas: as duas */
    CHECK(s && !linha_de_aresta(s, "723", "->", "500", NULL));
    free(s);
    descarregar(&c);
}

/* Sem sentido: graph, cada aresta uma vez só, mesmo com o par recíproco. */
static void simetrizada(GraphRep rep)
{
    Carregado c = carregar("500 723\n723 81\n81 500\n500 81\n", rep, GRAPH_SYMMETRIC);
    CHECK(dot_gravar(SAIDA, c.g, &c.ids, NULL, NULL) == DOT_OK);
    char *s = ler_saida();
    CHECK(s && strncmp(s, "graph ", 6) == 0);
    CHECK(s && contar(s, " -- ") == 3 && contar(s, " -> ") == 0);
    CHECK(s && (linha_de_aresta(s, "500", "--", "81", NULL) ||
                linha_de_aresta(s, "81", "--", "500", NULL)));
    free(s);
    descarregar(&c);
}

/* Vértice isolado também aparece no desenho. */
static void vertices_sem_aresta(GraphRep rep)
{
    Carregado c = carregar("1 2\n", rep, GRAPH_DIRECTED);
    Graph *g;
    graph_create(3, rep, GRAPH_DIRECTED, &g);   /* 3 vértices, só a aresta 0 -> 1 */
    graph_add_edge(g, 0, 1);
    idmap_inserir(&c.ids, 99, &(size_t){0});   /* índice 2 = ID 99, sem arestas */
    CHECK(dot_gravar(SAIDA, g, &c.ids, NULL, NULL) == DOT_OK);
    char *s = ler_saida();
    CHECK(s && strstr(s, "\"99\"") != NULL);
    free(s);
    graph_destroy(g);
    descarregar(&c);
}

/* Cores, vértices destacados e arestas destacadas, informadas por índice.
 * Na visão sem sentido, o destaque vale com as pontas em qualquer ordem. */
static void estilo(GraphRep rep)
{
    Carregado c = carregar("500 723\n723 81\n81 500\n", rep, GRAPH_SYMMETRIC);
    size_t i500 = indice(&c, 500), i723 = indice(&c, 723), i81 = indice(&c, 81);

    const char *cor[3] = {NULL, NULL, NULL};
    cor[i500] = "#2a78d6";
    unsigned char destaque[3] = {0, 0, 0};
    destaque[i723] = 1;
    /* índices: 81 = 0, 500 = 1, 723 = 2 (ordem de aparição nas arestas
     * ordenadas); a aresta {81, 723} vai como (2, 0), invertida de propósito */
    CHECK(i81 < i723);
    DotAresta arestas[] = {{i723, i81}};
    DotEstilo e = {cor, destaque, arestas, 1};

    CHECK(dot_gravar(SAIDA, c.g, &c.ids, &e, NULL) == DOT_OK);
    char *s = ler_saida();
    CHECK(s && strstr(s, "\"500\" [") && strstr(s, "fillcolor=\"#2a78d6\""));
    CHECK(s && contar(s, "fillcolor=") == 1);
    CHECK(s && contar(s, "penwidth=3") == 1);   /* só o 723 */
    int destacada = s && (linha_de_aresta(s, "723", "--", "81", DOT_COR_DESTAQUE) ||
                          linha_de_aresta(s, "81", "--", "723", DOT_COR_DESTAQUE));
    CHECK(destacada);
    CHECK(s && contar(s, "color=\"" DOT_COR_DESTAQUE "\"") >= 1);
    int outra_sem = s && (linha_de_aresta(s, "500", "--", "723", NULL) ||
                          linha_de_aresta(s, "723", "--", "500", NULL)) &&
                    !linha_de_aresta(s, "500", "--", "723", DOT_COR_DESTAQUE) &&
                    !linha_de_aresta(s, "723", "--", "500", DOT_COR_DESTAQUE);
    CHECK(outra_sem);
    free(s);
    descarregar(&c);
}

/* Na direcionada, o destaque respeita o sentido: destacar 723 -> 81 não
 * destaca 81 -> 723. */
static void destaque_respeita_sentido(GraphRep rep)
{
    Carregado c = carregar("723 81\n81 723\n", rep, GRAPH_DIRECTED);
    DotAresta arestas[] = {{indice(&c, 723), indice(&c, 81)}};
    DotEstilo e = {NULL, NULL, arestas, 1};
    CHECK(dot_gravar(SAIDA, c.g, &c.ids, &e, NULL) == DOT_OK);
    char *s = ler_saida();
    CHECK(s && linha_de_aresta(s, "723", "->", "81", DOT_COR_DESTAQUE));
    CHECK(s && linha_de_aresta(s, "81", "->", "723", NULL) &&
          !linha_de_aresta(s, "81", "->", "723", DOT_COR_DESTAQUE));
    free(s);
    descarregar(&c);
}

/* O título vira rótulo do grafo, com aspas e barras escapadas. */
static void titulo_escapado(GraphRep rep)
{
    Carregado c = carregar("1 2\n", rep, GRAPH_DIRECTED);
    CHECK(dot_gravar(SAIDA, c.g, &c.ids, NULL, "BFS \"a partir\" de C:\\x") == DOT_OK);
    char *s = ler_saida();
    CHECK(s && strstr(s, "label=\"BFS \\\"a partir\\\" de C:\\\\x\"") != NULL);
    free(s);
    descarregar(&c);
}

static void destino_invalido(GraphRep rep)
{
    Carregado c = carregar("1 2\n", rep, GRAPH_DIRECTED);
    CHECK(dot_gravar("bin/nao_existe/x.dot", c.g, &c.ids, NULL, NULL) == DOT_ERRO_ARQUIVO);
    CHECK(dot_gravar("/dev/full", c.g, &c.ids, NULL, NULL) == DOT_ERRO_ARQUIVO);
    descarregar(&c);
}

/* Até 30 vértices, o layout padrão (camadas); acima, o arquivo pede o sfdp,
 * por forças, porque o desenho em camadas de uma rede densa fica ilegível e
 * leva dezenas de segundos para sair. */
static void layout_por_tamanho(GraphRep rep)
{
    Carregado c = carregar("1 2\n", rep, GRAPH_DIRECTED);
    CHECK(dot_gravar(SAIDA, c.g, &c.ids, NULL, NULL) == DOT_OK);
    char *s = ler_saida();
    CHECK(s && strstr(s, "layout=") == NULL);
    free(s);
    descarregar(&c);

    for (size_t n = 30; n <= 31; n++) {
        Graph *g;
        IdMap ids;
        idmap_iniciar(&ids);
        size_t i;
        for (unsigned long long id = 0; id < n; id++)
            idmap_inserir(&ids, id, &i);
        graph_create(n, rep, GRAPH_SYMMETRIC, &g);
        CHECK(dot_gravar(SAIDA, g, &ids, NULL, NULL) == DOT_OK);
        s = ler_saida();
        CHECK(s && (strstr(s, "layout=sfdp") != NULL) == (n > 30));
        free(s);
        graph_destroy(g);
        idmap_liberar(&ids);
    }
}

static void rodar(GraphRep rep)
{
    direcionada(rep);
    simetrizada(rep);
    vertices_sem_aresta(rep);
    estilo(rep);
    destaque_respeita_sentido(rep);
    titulo_escapado(rep);
    destino_invalido(rep);
    layout_por_tamanho(rep);
}

int main(void)
{
    rodar(GRAPH_LIST);
    rodar(GRAPH_MATRIX);
    CHECK(mt_current_bytes() == 0);

    remove(ENTRADA);
    remove(SAIDA);
    printf("%d verificações, %d falha(s)\n", verificacoes, falhas);
    return falhas ? 1 : 0;
}
