# PI-3B — Análise de Redes Sociais e Disseminação de Informação

Projeto Integrador da disciplina de **Teoria dos Grafos** — estudo de caso 9.

Modelagem de uma rede social real como grafo para investigar **como a informação se
espalha**: quão distantes os usuários estão entre si, que parcelas da rede ficam
inalcançáveis a partir de um ponto, e quais usuários funcionam como pontes cuja
remoção fragmenta a rede.

> **Status:** Fase I em desenvolvimento. Ainda não há binário utilizável — as seções de
> compilação e uso abaixo descrevem a interface planejada, não o que já roda.
> O andamento fica no [board do projeto](https://github.com/users/joaopedrocampos1/projects/4)
> e nas [issues](https://github.com/joaopedrocampos1/PI-3B/issues).

## Modelagem

| Elemento | Representa |
|---|---|
| Vértice | Uma conta / usuário da rede |
| Aresta | Uma relação entre duas contas (amizade, seguimento ou interação) |

O direcionamento das arestas depende da base escolhida e será fixado junto com o
dataset. A Fase I trata o grafo como **não valorado**: nenhuma aresta tem peso, e o
interesse é puramente topológico. Pesos entram só na Fase II.

## Perguntas que a Fase I pretende responder

- Qual a distância média entre dois usuários, e quantos saltos são necessários para
  alcançar toda a rede a partir de um único ponto de partida?
- A rede é conexa? Qual o tamanho do componente gigante e quanto público fica fora
  do alcance de uma campanha que comece dentro dele?
- Existem ciclos de propagação, em que a informação retorna à origem?
- O grafo é bipartido — ou seja, dá para separar os usuários em dois grupos sem
  relação interna?
- Quais são os usuários-ponte e os canais críticos, cuja remoção isola grupos
  inteiros?

## Requisitos

| ID | Requisito |
|---|---|
| RF01 | Carregar grafos de datasets reais com no mínimo 1.000 vértices |
| RF02 | Alternar entre Lista e Matriz de Adjacência para comparar consumo de memória |
| RF03 | Registrar tempo de execução (ms) e consumo de memória de cada algoritmo |
| RNF01 | Implementação autoral em C, sem bibliotecas prontas de grafos nos algoritmos core |

Scripts auxiliares de geração de gráficos podem usar Python ou gnuplot — a restrição
do RNF01 vale para os algoritmos de grafos, não para o ferramental de análise.

## Algoritmos da Fase I

| Algoritmo | Pergunta que responde |
|---|---|
| BFS | Graus de separação, distância média, excentricidade |
| DFS iterativa | Base para ciclos e classificação de arestas |
| Componentes conexos | Fragmentação da rede, componente gigante |
| Detecção de ciclos | Caminhos de retorno da informação |
| Verificação de bipartição | Separabilidade em dois grupos |
| Tarjan | Pontes e vértices de articulação |

A DFS é implementada com **pilha explícita**, não por recursão: com dezenas de
milhares de vértices a versão recursiva estoura a pilha de chamadas.

## Estrutura do repositório

```
├── include/          headers públicos (graph.h define a interface comum)
├── src/              todo o código C
├── data/
│   ├── raw/          dataset original, não modificado
│   └── samples/      subgrafos amostrados (N = 100, 250, 500, 1.000)
├── results/
│   ├── log.csv       uma linha por execução de algoritmo (RF03)
│   ├── dot/          grafos exportados para visualização
│   └── figs/         imagens renderizadas e gráficos do artigo
├── scripts/          geração de gráficos e renderização dos .dot
├── tests/            casos com resultado conhecido
├── docs/             artigo no template SBC
└── bin/              binário compilado (fora do controle de versão)
```

`src/` e `include/` são planos: o nome do arquivo já diz a que módulo ele pertence,
sem precisar de subpastas espelhadas.

| Prefixo | Módulo | Responsável |
|---|---|---|
| `edgelist.c` `idmap.c` `dot.c` | leitura, remapeamento de IDs e exportação Graphviz | DEV1 |
| `graph.c` `graph_list.c` `graph_matrix.c` | as duas representações e o despacho entre elas | DEV2 |
| `bfs.c` `components.c` `bipartite.c` | buscas em largura e conectividade | DEV3 |
| `dfs.c` `cycles.c` `tarjan.c` | busca em profundidade e algoritmos estruturais | DEV4 |
| `timer.c` `memtrack.c` `logger.c` | instrumentação de tempo, memória e log | DEV4 |
| `subgraph.c` | amostragem para o protocolo experimental | DEV1 |

A separação por arquivo importa mais do que parece: com cinco pessoas commitando em
paralelo por quatro semanas, cada um mexendo nos seus arquivos reduz conflito de merge.
O `graph.h` é a única fronteira realmente compartilhada — por isso mudanças nele exigem
revisão de mais de uma pessoa.

## Visualização

Os grafos são exportados em formato Graphviz (`.dot`) para `results/dot/`, e
renderizados com `scripts/render_dot.sh`.

A visualização só é legível em grafos pequenos: com mais de algumas centenas de
vértices o resultado vira uma mancha. Ela é usada, portanto, nos subgrafos amostrados,
nos casos de teste de resultado conhecido — onde a imagem serve como evidência de que
o algoritmo acertou — e em vizinhanças específicas, como o entorno de um vértice de
articulação, mostrando a fragmentação que sua remoção provoca.

A exportação colore os vértices e arestas conforme o resultado do algoritmo executado:
componentes conexos em cores distintas, as duas classes da bipartição em cores opostas,
pontes destacadas.

## Compilação

Requer `gcc` e `make`. Nenhuma dependência externa.

```bash
make
```

O projeto deve compilar sem nenhum aviso sob `-Wall -Wextra` e rodar sem vazamentos
sob `valgrind`.

## Uso

```bash
./bin/grafos --input data/rede.txt --struct lista --algo bfs --source 42
```

| Flag | Descrição |
|---|---|
| `--input` | Arquivo de entrada (edge list em CSV ou TXT) |
| `--struct` | `lista` ou `matriz` — alternável sem recompilar (RF02) |
| `--algo` | `bfs`, `dfs`, `componentes`, `ciclos`, `bipartido`, `articulacao` |
| `--source` | Vértice de origem, para os algoritmos que precisam de um |
| `--output` | Arquivo de saída dos resultados |

Toda execução acrescenta uma linha a `results/log.csv` com timestamp, dataset, N, M,
estrutura, algoritmo, tempo em ms, memória em KB e número da repetição (RF03).

## Formato de entrada

Edge list, uma aresta por linha:

```
# linhas iniciadas por # são comentários
0 1
0 2
1 2
```

O leitor aceita separadores variados, ignora comentários e arestas duplicadas, e
reporta linhas malformadas. Como IDs de datasets reais costumam ser esparsos, eles são
remapeados internamente para o intervalo `0..V-1`, preservando o mapa inverso para
exibição.

## Dataset

Ainda não definido — a seleção está na
[issue #4](https://github.com/joaopedrocampos1/PI-3B/issues/4). Candidatos em avaliação:
SNAP (ego-Facebook, Twitter combined, Slashdot, Epinions) e Kaggle. Os critérios são
mínimo de 1.000 vértices, formato aberto, licença compatível com uso acadêmico e
origem documentada.

## Protocolo experimental

Cada algoritmo é executado sobre subgrafos de **N = 100, 250, 500, 1.000 e o grafo
completo**, nas duas representações, com repetições por configuração (descartando a
primeira execução como aquecimento) e registro de média e desvio padrão.

Os resultados alimentam duas análises: o crescimento do tempo de execução contra
O(|V| + |E|), e a comparação de memória entre lista e matriz de adjacência. A matriz
tem custo O(V²) e tende a se tornar inviável em redes sociais reais, que são esparsas
— confirmar esse limite experimentalmente é parte do resultado, não um problema.

## Fase II

Fora do escopo atual. Introduzirá arestas valoradas, algoritmos de menor caminho
(Dijkstra ou Bellman-Ford), árvore geradora mínima (Kruskal ou Prim) e um problema
NP-difícil do domínio, com solução exata e heurística comparadas.

A interface única definida em `include/graph.h` é o que permite reaproveitar as duas
representações nessa fase sem reescrever a base.

## Equipe

Cinco integrantes, com papéis de PO e Scrum Master rotativos:

| Papel | Responsabilidade |
|---|---|
| DEV1 | Engenharia de dados: dataset, parser, amostragem |
| DEV2 | Estruturas de dados: TAD Grafo, lista, matriz, medição de memória |
| DEV3 | Buscas: BFS, graus de separação, componentes, bipartição |
| DEV4 | Algoritmos estruturais e instrumentação: DFS, ciclos, Tarjan, logging |
| DEV5 | QA, experimentos e artigo |

## Entregas

- Aplicação em C executável localmente
- Artigo científico de 4 a 6 páginas no template SBC, em `docs/`
- Histórico de commits coerente com a evolução do projeto
