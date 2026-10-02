#include "bfs.h"
#include "cli.h"
#include "edgelist.h"
#include "graph.h"
#include "idmap.h"
#include "logger.h"
#include "memtrack.h"
#include "separacao.h"
#include "subgraph.h"
#include "timer.h"

#include <stdint.h>
#include <stdio.h>

/* Protocolo experimental: 3 sementes x 4 tamanhos de amostra. */
static const size_t TAMANHOS[] = {100, 250, 500, 1000};
#define NUM_TAMANHOS (sizeof TAMANHOS / sizeof TAMANHOS[0])
#define NUM_SEMENTES 3

/* Número de vértices distintos entre as arestas lidas. */
static int contar_vertices(const EdgeList *el, size_t *v)
{
    IdMap m;
    idmap_iniciar(&m);
    size_t i;
    for (size_t e = 0; e < el->n; e++)
        if (!idmap_inserir(&m, el->arestas[e].origem, &i) ||
            !idmap_inserir(&m, el->arestas[e].destino, &i)) {
            idmap_liberar(&m);
            return 0;
        }
    *v = m.n;
    idmap_liberar(&m);
    return 1;
}

static void resumir(const char *caminho, const EdgeList *el, size_t v)
{
    printf("%s: %zu vértices, %zu arestas\n", caminho, v, el->n);
    if (el->duplicadas || el->lacos)
        printf("  descartadas: %zu duplicadas, %zu laços\n", el->duplicadas, el->lacos);
    if (el->malformadas)
        printf("  ignoradas: %zu linhas malformadas (a primeira na linha %zu)\n",
               el->malformadas, el->primeira_malformada);
}

/* Grava em op->pasta_amostras um arquivo por semente e tamanho. Devolve o
 * código de saída do programa. */
static int gerar_amostras(const EdgeList *el, const Opcoes *op)
{
    Amostrador a;
    if (subgraph_preparar(&a, el) != AMOSTRA_OK) {
        fprintf(stderr, "erro: memória insuficiente para preparar a amostragem\n");
        return 1;
    }

    unsigned long long sementes[NUM_SEMENTES];
    AmostraStatus st = subgraph_sortear_sementes(&a, op->semente_rng, NUM_SEMENTES, sementes);
    if (st != AMOSTRA_OK) {
        fprintf(stderr, "erro: %s\n", st == AMOSTRA_SEMENTES_INSUFICIENTES
                                          ? "o grafo tem menos vértices que o número de sementes"
                                          : "memória insuficiente para sortear as sementes");
        subgraph_liberar(&a);
        return 1;
    }

    int codigo = 0;
    for (size_t s = 0; s < NUM_SEMENTES && codigo != 1; s++) {
        for (size_t t = 0; t < NUM_TAMANHOS; t++) {
            size_t n = TAMANHOS[t];
            EdgeList sub;
            st = subgraph_bfs(&a, sementes[s], n, &sub);
            if (st == AMOSTRA_COMPONENTE_PEQUENO) {
                fprintf(stderr, "aviso: a semente %llu alcança menos de %zu vértices; amostra pulada\n",
                        sementes[s], n);
                codigo = 2;
                continue;
            }
            if (st != AMOSTRA_OK) {
                fprintf(stderr, "erro: memória insuficiente para gerar a amostra\n");
                codigo = 1;
                break;
            }

            char caminho[1024], comentario[1024];
            int c1 = snprintf(caminho, sizeof caminho, "%s/bfs_%llu_n%zu.txt",
                              op->pasta_amostras, sementes[s], n);
            snprintf(comentario, sizeof comentario,
                     "subgrafo induzido pelos %zu primeiros vértices de um BFS sem sentido\n"
                     "dataset: %s\nsemente-rng: %llu\nsemente: %llu\nN: %zu\narestas: %zu",
                     n, op->entrada, op->semente_rng, sementes[s], n, sub.n);

            if (c1 < 0 || (size_t)c1 >= sizeof caminho ||
                edgelist_gravar(caminho, &sub, comentario) != EDGELIST_OK) {
                fprintf(stderr, "erro: não foi possível gravar '%s' (a pasta existe?)\n", caminho);
                edgelist_liberar(&sub);
                codigo = 1;
                break;
            }
            printf("%s: %zu vértices, %zu arestas\n", caminho, n, sub.n);
            edgelist_liberar(&sub);
        }
    }

    subgraph_liberar(&a);
    return codigo;
}

/*
 * Medição de cada execução de --algo (#25), como fixado no protocolo da
 * Metodologia do artigo (#26):
 *  - tempo: só o algoritmo; leitura, remapeamento e construção ficam fora;
 *  - memória: pico do memtrack entre o início da construção do grafo e o fim
 *    do algoritmo. Quem executa chama mt_reset_peak() antes de graph_build e
 *    lê mt_peak_bytes() assim que o algoritmo termina.
 * Cada execução acrescenta uma linha a op->log.
 */
static int registrar(const Opcoes *op, const Graph *g, double tempo_ms, size_t pico_bytes)
{
    double memoria_kb = (double)pico_bytes / 1024.0;
    printf("  tempo: %.3f ms, memória: %.1f KB (execução %u)\n", tempo_ms, memoria_kb,
           op->execucao);

    LogRegistro r = {op->entrada, graph_num_vertices(g), graph_num_edges(g),
                     graph_rep_name(graph_rep(g)), cli_nome_algoritmo(op->algoritmo),
                     tempo_ms, memoria_kb, op->execucao};
    if (log_registrar(op->log, &r) != LOG_OK) {
        fprintf(stderr, "erro: não foi possível gravar em '%s' (a pasta existe?)\n", op->log);
        return 0;
    }
    return 1;
}

/* Quantos vértices estão a cada distância da origem. `hist` tem dist_max + 1
 * posições. */
static void histograma(const BfsResultado *r, size_t *hist)
{
    for (size_t d = 0; d <= r->dist_max; d++)
        hist[d] = 0;
    for (size_t v = 0; v < r->n; v++)
        if (r->visitado[v])
            hist[r->dist[v]]++;
}

/* --algo bfs: BFS na visão direcionada a partir de --source. Mostra o alcance
 * e, com --output, grava o histograma de distâncias em CSV. Devolve o código
 * de saída do programa. */
static int executar_bfs(const EdgeList *el, const Opcoes *op)
{
    if (!op->tem_origem) {
        fprintf(stderr, "erro: --algo bfs precisa de --source <id>\n");
        return 1;
    }
    GraphRep rep = op->estrutura == ESTRUTURA_MATRIZ ? GRAPH_MATRIX : GRAPH_LIST;
    IdMap ids;
    idmap_iniciar(&ids);
    Graph *g;
    mt_reset_peak();
    if (graph_build(el, &ids, rep, GRAPH_DIRECTED, &g) != GRAPH_OK) {
        fprintf(stderr, "erro: memória insuficiente para montar o grafo como %s\n",
                graph_rep_name(rep));
        idmap_liberar(&ids);
        return 1;
    }

    int codigo = 1;
    size_t origem;
    BfsResultado r;
    size_t *hist = NULL;
    if (!idmap_buscar(&ids, op->origem, &origem)) {
        fprintf(stderr, "erro: o vértice %llu não existe no grafo\n", op->origem);
        goto fim;
    }
    Timer *t = timer_criar();
    if (!t) {
        fprintf(stderr, "erro: memória insuficiente para o cronômetro\n");
        goto fim;
    }
    timer_iniciar(t);
    BfsStatus bs = bfs_executar(g, origem, &r);
    double ms = timer_parar_ms(t);
    size_t pico = mt_peak_bytes();
    timer_destruir(t);
    if (bs != BFS_OK) {
        fprintf(stderr, "erro: memória insuficiente para o BFS\n");
        goto fim;
    }
    hist = mt_malloc((r.dist_max + 1) * sizeof *hist);
    if (!hist) {
        fprintf(stderr, "erro: memória insuficiente para o histograma\n");
        bfs_liberar(&r);
        goto fim;
    }
    histograma(&r, hist);

    size_t n = graph_num_vertices(g);
    printf("BFS a partir de %llu (%s, visão direcionada)\n", op->origem, graph_rep_name(rep));
    printf("  alcançados: %zu de %zu vértices (%.2f%%)\n", r.alcancados, n,
           100.0 * (double)r.alcancados / (double)n);
    printf("  excentricidade: %zu\n", r.dist_max);
    printf("  distância média: %.4f\n", r.dist_media);
    for (size_t d = 1; d <= r.dist_max; d++)
        printf("  %zu salto(s): %zu vértices\n", d, hist[d]);

    codigo = registrar(op, g, ms, pico) ? 0 : 1;
    if (op->saida) {
        FILE *f = fopen(op->saida, "w");
        if (!f) {
            fprintf(stderr, "erro: não foi possível gravar '%s'\n", op->saida);
            codigo = 1;
        } else {
            fprintf(f, "distancia,vertices\n");
            for (size_t d = 0; d <= r.dist_max; d++)
                fprintf(f, "%zu,%zu\n", d, hist[d]);
            fclose(f);
        }
    }
    mt_free(hist);
    bfs_liberar(&r);
fim:
    graph_destroy(g);
    idmap_liberar(&ids);
    return codigo;
}

/* Até este tamanho, --algo separacao também calcula diâmetro e distância média
 * exatos (BFS de todos os vértices). Cobre todas as amostras de data/samples. */
#define SEPARACAO_LIMITE_EXATO 5000

/* Grava <prefixo>_histograma.csv e <prefixo>_perfis.csv. */
static int gravar_separacao(const char *prefixo, const SeparacaoResultado *r, const IdMap *ids)
{
    char caminho[1024];
    int c = snprintf(caminho, sizeof caminho, "%s_histograma.csv", prefixo);
    FILE *f = (c > 0 && (size_t)c < sizeof caminho) ? fopen(caminho, "w") : NULL;
    if (!f) {
        fprintf(stderr, "erro: não foi possível gravar '%s'\n", caminho);
        return 0;
    }
    /* pares_rede: pares (perfil, vértice alcançado) a essa distância;
     * perfis_influenciador: perfis cujo influenciador mais próximo está a essa distância */
    fprintf(f, "saltos,pares_rede,perfis_influenciador\n");
    for (size_t d = 1; d <= r->diametro_estimado; d++)
        fprintf(f, "%zu,%zu,%zu\n", d, r->hist_rede[d], r->hist_influenciador[d]);
    fclose(f);

    c = snprintf(caminho, sizeof caminho, "%s_perfis.csv", prefixo);
    f = (c > 0 && (size_t)c < sizeof caminho) ? fopen(caminho, "w") : NULL;
    if (!f) {
        fprintf(stderr, "erro: não foi possível gravar '%s'\n", caminho);
        return 0;
    }
    fprintf(f, "perfil,alcancados,excentricidade,dist_media,dist_influenciador_mais_proximo,"
               "influenciadores_alcancados,dist_media_influenciadores\n");
    for (size_t i = 0; i < r->num_perfis; i++) {
        const SeparacaoPerfil *p = &r->perfis[i];
        fprintf(f, "%llu,%zu,%zu,%.4f,", idmap_original(ids, p->vertice), p->alcancados,
                p->excentricidade, p->dist_media);
        if (p->dist_influenciador == SIZE_MAX)
            fprintf(f, ",");   /* nenhum influenciador alcançável: campo vazio */
        else
            fprintf(f, "%zu,", p->dist_influenciador);
        fprintf(f, "%zu,%.4f\n", p->influenciadores_alcancados, p->dist_media_influenciadores);
    }
    fclose(f);
    return 1;
}

/* --algo separacao: graus de separação entre perfis comuns e influenciadores
 * (#17), na visão direcionada. Devolve o código de saída do programa. */
static int executar_separacao(const EdgeList *el, const Opcoes *op)
{
    GraphRep rep = op->estrutura == ESTRUTURA_MATRIZ ? GRAPH_MATRIX : GRAPH_LIST;
    IdMap ids;
    idmap_iniciar(&ids);
    Graph *g;
    mt_reset_peak();
    if (graph_build(el, &ids, rep, GRAPH_DIRECTED, &g) != GRAPH_OK) {
        fprintf(stderr, "erro: memória insuficiente para montar o grafo como %s\n",
                graph_rep_name(rep));
        idmap_liberar(&ids);
        return 1;
    }

    Timer *t = timer_criar();
    if (!t) {
        fprintf(stderr, "erro: memória insuficiente para o cronômetro\n");
        graph_destroy(g);
        idmap_liberar(&ids);
        return 1;
    }
    /* o cálculo exato, feito à parte nas amostras pequenas, fica fora da medição */
    SeparacaoResultado r;
    timer_iniciar(t);
    SeparacaoStatus st = separacao_analisar(g, SEPARACAO_FRACAO_PADRAO, SEPARACAO_PERFIS_PADRAO,
                                            op->semente_rng, &r);
    double ms = timer_parar_ms(t);
    size_t pico = mt_peak_bytes();
    timer_destruir(t);
    if (st != SEPARACAO_OK) {
        fprintf(stderr, "erro: %s\n", st == SEPARACAO_ERRO_MEMORIA
                                          ? "memória insuficiente para a análise"
                                          : "o grafo não tem perfis comuns para analisar");
        graph_destroy(g);
        idmap_liberar(&ids);
        return 1;
    }

    size_t n = graph_num_vertices(g);
    printf("Graus de separação (%s, visão direcionada, semente %llu)\n", graph_rep_name(rep),
           op->semente_rng);
    printf("  influenciadores: %zu (%.0f%% com mais seguidores; pelo menos %zu seguidores)\n",
           r.num_influenciadores, 100.0 * SEPARACAO_FRACAO_PADRAO, r.grau_entrada_corte);
    printf("  perfis comuns analisados: %zu\n", r.num_perfis);
    printf("  perfis que não alcançam nenhum influenciador: %zu (%.2f%%)\n",
           r.perfis_sem_influenciador, 100.0 * (double)r.perfis_sem_influenciador / (double)r.num_perfis);
    printf("  distância média perfil comum -> influenciador mais próximo: %.4f\n",
           r.dist_media_influenciador);
    printf("  distância média perfil comum -> influenciadores (todos os alcançados): %.4f\n",
           r.dist_media_conjunto);
    printf("  distância média geral da rede (a partir dos perfis): %.4f\n", r.dist_media_rede);
    printf("  diâmetro estimado (maior excentricidade entre os perfis): %zu\n",
           r.diametro_estimado);

    int codigo = 0;
    if (n <= SEPARACAO_LIMITE_EXATO) {
        size_t diam;
        double media;
        if (separacao_exato(g, &diam, &media) == SEPARACAO_OK)
            printf("  exato (BFS de todos os %zu vértices): diâmetro %zu, distância média %.4f\n",
                   n, diam, media);
        else {
            fprintf(stderr, "erro: memória insuficiente para o cálculo exato\n");
            codigo = 1;
        }
    }
    if (op->saida && !gravar_separacao(op->saida, &r, &ids))
        codigo = 1;
    if (!registrar(op, g, ms, pico))
        codigo = 1;

    separacao_liberar(&r);
    graph_destroy(g);
    idmap_liberar(&ids);
    return codigo;
}

int main(int argc, char **argv)
{
    Opcoes op;
    char erro[256];
    CliStatus st = cli_ler(argc, argv, &op, erro, sizeof erro);
    if (st == CLI_AJUDA) {
        cli_uso(stdout, argv[0]);
        return 0;
    }
    if (st == CLI_ERRO) {
        fprintf(stderr, "erro: %s\n\n", erro);
        cli_uso(stderr, argv[0]);
        return 1;
    }

    EdgeList el;
    EdgeListStatus le = edgelist_ler(op.entrada, &el);
    if (le != EDGELIST_OK) {
        fprintf(stderr, "erro: %s '%s'\n",
                le == EDGELIST_ERRO_MEMORIA ? "memória insuficiente para ler"
                                            : "não foi possível ler",
                op.entrada);
        return 1;
    }

    size_t v;
    if (!contar_vertices(&el, &v)) {
        fprintf(stderr, "erro: memória insuficiente para mapear os vértices\n");
        edgelist_liberar(&el);
        return 1;
    }
    resumir(op.entrada, &el, v);

    int codigo = 0;
    if (op.pasta_amostras) {
        codigo = gerar_amostras(&el, &op);
    } else if (op.algoritmo == ALGO_BFS) {
        codigo = executar_bfs(&el, &op);
    } else if (op.algoritmo == ALGO_SEPARACAO) {
        codigo = executar_separacao(&el, &op);
    } else if (op.algoritmo != ALGO_NENHUM) {
        fprintf(stderr, "o algoritmo '%s' ainda não foi implementado\n",
                cli_nome_algoritmo(op.algoritmo));
        codigo = 2;
    }

    edgelist_liberar(&el);
    return codigo;
}
