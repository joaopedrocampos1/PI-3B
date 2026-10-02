# PI-3B — Análise de Redes Sociais e Disseminação de Informação

Projeto Integrador da disciplina de **Teoria dos Grafos** — estudo de caso 9.

Modelagem de uma rede social real como grafo para investigar **como a informação se
espalha**: quão distantes os usuários estão entre si, que parcelas da rede ficam
inalcançáveis a partir de um ponto, e quais usuários funcionam como pontes cuja
remoção fragmenta a rede.

> **Status:** Fase I em desenvolvimento. O programa lê o dataset, gera as amostras do
> protocolo experimental, monta o grafo como lista ou matriz e executa os seis algoritmos
> da Fase I (BFS, DFS, componentes conexos, ciclos, bipartição, pontes e articulação),
> além dos graus de separação, registrando tempo e memória de cada execução.
> O andamento fica no [board do projeto](https://github.com/users/joaopedrocampos1/projects/4)
> e nas [issues](https://github.com/joaopedrocampos1/PI-3B/issues).

## Modelagem

| Elemento | Representa |
|---|---|
| Vértice | Um usuário do Twitter |
| Aresta A → B | O usuário A segue o usuário B |

O grafo é **direcionado**, e a Fase I o trata como **não valorado**: nenhuma aresta tem
peso, e o interesse é puramente topológico. Pesos entram só na Fase II.

Como nem todo algoritmo é definido para grafos direcionados, a mesma base é lida de
três formas:

| Visão | Usada em |
|---|---|
| Direcionada | BFS e graus de separação, detecção de ciclos |
| Simetrizada (A → B vira A — B) | bipartição, pontes e vértices de articulação |
| Recíproca (só pares que se seguem mutuamente) | Clique Máxima, na Fase II |

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
| Componentes conexos (Tarjan) | Fortemente conexos: quem alcança quem e o público inalcançável; fracamente conexos: se a rede é uma peça só |
| Detecção de ciclos | Caminhos de retorno da informação |
| Verificação de bipartição | Separabilidade em dois grupos |
| Tarjan | Pontes e vértices de articulação |

A DFS é implementada com **pilha explícita**, não por recursão: com dezenas de
milhares de vértices a versão recursiva estoura a pilha de chamadas.

## Estrutura do repositório

```
├── include/
├── src/
├── data/
│   ├── raw/
│   └── samples/
├── results/
│   ├── log.csv
│   ├── dot/
│   └── figs/
├── scripts/
├── tests/
├── docs/
└── bin/
```

Em uma frase: `data/` é o que entra, `src/` e `include/` são o que você escreve,
`results/` é o que sai, e `scripts/`, `tests/` e `docs/` são o apoio para analisar,
validar e escrever.

**`include/`** — os headers (`.h`). O principal é o `graph.h`, que declara a interface
do grafo (criar, inserir aresta, vizinhos, grau) sem dizer se por baixo é lista ou
matriz. É isso que permite trocar a estrutura em tempo de execução (RF02), e é o que os
algoritmos enxergam.

**`src/`** — todo o código C: parser, as duas representações, os seis algoritmos,
instrumentação e o `main.c`.

**`data/raw/`** — o dataset original, **nunca editado**. Se algum arquivo processado for
corrompido, é daqui que se recomeça.

**`data/samples/`** — os subgrafos de N = 100, 250, 500 e 1.000 gerados a partir do raw,
um arquivo por semente e tamanho (`bfs_<semente>_n<N>.txt`). Ficam separados porque são
derivados: podem ser apagados e regerados com `--amostrar`, que produz exatamente os
mesmos arquivos. São versionados para que todos rodem os experimentos sobre as mesmas
amostras.

**`results/log.csv`** — uma linha por execução de algoritmo, com tempo em milissegundos
e consumo de memória (RF03). É a matéria-prima dos gráficos e a evidência experimental
do artigo, por isso é versionado.

**`results/dot/`** — os grafos exportados em formato Graphviz, para visualização.

**`results/figs/`** — as imagens: os `.dot` renderizados e os gráficos de desempenho.
É de onde as figuras saem para dentro do artigo.

**`scripts/`** — Python/gnuplot para ler o `log.csv` e desenhar os gráficos, mais a
renderização dos `.dot` em lote. Fica separado de propósito: o RNF01 proíbe biblioteca
pronta de grafos nos algoritmos, mas não no ferramental de análise, e essa fronteira
precisa ser visível.

**`tests/`** — os grafos de resultado conhecido (linha, ciclo, estrela, desconexo,
bipartido). São pequenos e a resposta certa é sabida de antemão, então servem para
provar que o algoritmo está correto antes de soltá-lo sobre 1.000 vértices.

**`docs/`** — o artigo no template SBC.

**`bin/`** — o executável compilado. Está no `.gitignore`: binário não se versiona, cada
um gera o seu.

### Organização de `src/`

`src/` e `include/` são planos — o nome do arquivo já diz a que módulo ele pertence,
sem subpastas espelhadas.

| Prefixo | Módulo | Responsável |
|---|---|---|
| `edgelist.c` `idmap.c` | leitura da edge list e remapeamento de IDs | DEV1 |
| `subgraph.c` | amostragem para o protocolo experimental | DEV1 |
| `graph.c` `graph_list.c` `graph_matrix.c` | as duas representações e o despacho entre elas | DEV2 |
| `bfs.c` `separacao.c` `componentes.c` `bipartido.c` | buscas em largura, graus de separação, conectividade e bipartição | DEV3 |
| `dfs.c` `cycles.c` `tarjan.c` | busca em profundidade e algoritmos estruturais | DEV4 |
| `timer.c` `memtrack.c` `logger.c` | instrumentação de tempo, memória e log | DEV4 |
| `cli.c` `main.c` | linha de comando e ligação dos módulos | — |

A separação por arquivo importa mais do que parece: com cinco pessoas commitando em
paralelo por quatro semanas, cada um mexendo nos seus arquivos reduz conflito de merge.
O `graph.h` é a única fronteira realmente compartilhada — por isso mudanças nele exigem
revisão de mais de uma pessoa.

## Visualização

> Planejada, ainda não implementada: `results/dot/` e `results/figs/` existem, mas o
> programa ainda não exporta `.dot`.

Os grafos serão exportados em formato Graphviz (`.dot`) para `results/dot/` e
renderizados em lote por um script em `scripts/`.

A visualização só é legível em grafos pequenos: com mais de algumas centenas de
vértices o resultado vira uma mancha. Ela é usada, portanto, nos subgrafos amostrados,
nos casos de teste de resultado conhecido — onde a imagem serve como evidência de que
o algoritmo acertou — e em vizinhanças específicas, como o entorno de um vértice de
articulação, mostrando a fragmentação que sua remoção provoca.

A exportação colore os vértices e arestas conforme o resultado do algoritmo executado:
componentes conexos em cores distintas, as duas classes da bipartição em cores opostas,
pontes destacadas.

## Do zero à primeira execução

### Pré-requisitos

- **Sistema:** Linux, macOS ou WSL no Windows. O programa usa chamadas POSIX
  (`clock_gettime`) e não compila nativamente no Windows (MinGW/MSVC).
- **Compilação:** `gcc` com suporte a C11 e `make`.
- **Dataset** (opcional, só para o grafo completo): `curl`, `sha256sum` e `gunzip`.
  As 12 amostras do protocolo já vêm no repositório.
- **Opcional:** `valgrind`, para conferir vazamentos.

No Ubuntu/Debian (inclusive no WSL), tudo isso se instala com:

```bash
sudo apt install build-essential curl valgrind
```

### Passo a passo

```bash
git clone https://github.com/joaopedrocampos1/PI-3B.git
cd PI-3B

make                          # compila bin/grafos
make test                     # roda os testes; deve terminar com "0 falha(s)" em todos

# primeira execução, sobre uma amostra versionada (não precisa do dataset)
./bin/grafos --input data/samples/bfs_307642294_n1000.txt --algo componentes

# para usar o grafo completo, baixe o dataset (44 MB descompactado)
./scripts/baixar_dataset.sh
./bin/grafos --input data/raw/twitter_combined.txt
```

A execução de `componentes` imprime algo como:

```
data/samples/bfs_307642294_n1000.txt: 1000 vértices, 38064 arestas
Componentes fortemente conexos (lista, visão direcionada)
  componentes: ...
  tempo: ... ms | memória: ... KB (execução 1)
...
```

e acrescenta duas linhas a `results/log.csv`.

## Compilação

Requer `gcc` e `make`. Nenhuma dependência externa.

```bash
make          # compila bin/grafos
make test     # compila e roda os testes de tests/ (o mesmo que ./tests/run_tests.sh)
make clean
```

A compilação usa `-std=c11 -Wall -Wextra -pedantic -Werror`: qualquer aviso interrompe o
build. O código também deve rodar sem vazamentos sob `valgrind`.

Um dos testes usa o dataset completo. Sem ele (veja [Dataset](#dataset)), esse teste
aparece como pulado e os demais rodam normalmente.

## Uso

```bash
# lê o arquivo e mostra um resumo (vértices, arestas, linhas descartadas)
./bin/grafos --input data/raw/twitter_combined.txt

# gera as 12 amostras do protocolo experimental em data/samples
./bin/grafos --input data/raw/twitter_combined.txt --amostrar data/samples
```

| Opção | Descrição |
|---|---|
| `--input` | Arquivo de entrada (edge list em CSV ou TXT). Obrigatória |
| `--struct` | `lista` ou `matriz`, alternável sem recompilar (RF02). Padrão: `lista` |
| `--algo` | `bfs`, `dfs`, `componentes`, `ciclos`, `bipartido`, `articulacao`, `separacao` |
| `--source` | Vértice de origem (ID original), para os algoritmos que precisam de um |
| `--output` | Arquivo de saída dos resultados |
| `--amostrar` | Pasta onde gravar as amostras: 3 sementes sorteadas × N = 100, 250, 500 e 1.000 |
| `--semente-rng` | Semente do sorteio das amostras. Padrão: `2026`, que gera os arquivos versionados |
| `--execucao` | Número desta execução, gravado no log. A 1 é o aquecimento. Padrão: `1` |
| `--log` | CSV onde cada execução de `--algo` acrescenta uma linha. Padrão: `results/log.csv` |
| `--help` | Mostra a ajuda |

Toda opção aceita também a forma `--opcao=valor`.

Código de saída: `0` em caso de sucesso, `1` em erro (argumento inválido, arquivo
ilegível, vértice de origem inexistente), `2` quando o pedido não pôde ser atendido
por completo — uma amostra maior que o componente da semente.

```bash
# BFS a partir do usuário 307642294: alcance, excentricidade, distância média e
# histograma de distâncias (gravado em CSV com --output)
./bin/grafos --input data/samples/bfs_307642294_n1000.txt --algo bfs --source 307642294 \
             --struct matriz --output results/bfs_hist.csv
```

```bash
# graus de separação entre perfis comuns e influenciadores (1% com mais seguidores),
# a partir de 1.000 perfis sorteados; grava results/separacao_histograma.csv e
# results/separacao_perfis.csv. Método e resultados em results/separacao.md
./bin/grafos --input data/raw/twitter_combined.txt --algo separacao --output results/separacao
```

```bash
# pontes e vértices de articulação, com o ranking por impacto de fragmentação: quantos
# usuários perdem contato com o maior pedaço da rede sem cada articulação
./bin/grafos --input data/raw/twitter_combined.txt --algo articulacao --output results/articulacao.csv
```

Cada algoritmo roda sobre a visão do grafo definida para ele:

| `--algo` | Visão | Origem | O que mostra |
|---|---|---|---|
| `bfs` | direcionada | `--source` (obrigatória) | alcance, excentricidade, distância média, histograma |
| `dfs` | direcionada | `--source` (ou o primeiro vértice) | floresta da DFS: árvores e alcance da origem |
| `componentes` | direcionada e simetrizada | — | componentes fortemente e fracamente conexos |
| `ciclos` | direcionada | — | se há ciclo, um exemplo e a classificação das arestas |
| `bipartido` | simetrizada | — | se é bipartido; se não, um ciclo ímpar |
| `articulacao` | simetrizada | — | pontes e vértices de articulação, por impacto |
| `separacao` | direcionada | — | graus de separação entre perfis comuns e influenciadores |

Os algoritmos estruturais (DFS, ciclos, bipartição e articulação) percorrem o grafo
inteiro, sem parar no primeiro resultado, para que o custo seja O(V + E) em qualquer
instância, como o protocolo experimental mede.

Toda execução de `--algo` acrescenta uma linha a `results/log.csv` (RF03), com as
colunas `timestamp, dataset, N, M, estrutura, algoritmo, tempo_ms, memoria_kb,
execucao_num`. Seguindo o protocolo do artigo, o tempo cobre só o algoritmo, e a
memória é o pico do memtrack entre o início da construção do grafo e o fim do
algoritmo. Como cada execução é um processo separado, o número da execução vem de
`--execucao`:

```bash
# 11 execuções da mesma configuração; a 1ª é o aquecimento, descartado na análise
for e in $(seq 1 11); do
    ./bin/grafos --input data/samples/bfs_307642294_n1000.txt --algo bfs \
                 --source 307642294 --struct lista --execucao $e
done
```

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

## Formatos de saída

Os resultados vão para a saída padrão. Erros e avisos vão para a saída de erro.

| Arquivo | Gerado por | Colunas |
|---|---|---|
| `results/log.csv` (ou `--log`) | toda execução de `--algo` | `timestamp,dataset,N,M,estrutura,algoritmo,tempo_ms,memoria_kb,execucao_num` |
| `--output` | `bfs` | `distancia,vertices`: quantos vértices estão a cada distância da origem |
| `--output` | `componentes` | `tipo,tamanho,componentes`: `tipo` é `fortes` ou `fracos` |
| `--output` | `dfs` | `vertice,descoberta,finalizacao`: tempos de cada vértice, de 1 a 2V |
| `--output` | `ciclos` | `posicao,vertice`: o ciclo de exemplo, na ordem dos repasses |
| `--output` | `bipartido` | `vertice,cor`: a cor (0 ou 1) de cada vértice |
| `--output` | `articulacao` | `vertice,impacto,pedacos`: as articulações, da de maior impacto para a de menor |
| `<prefixo>_histograma.csv` | `separacao` com `--output <prefixo>` | `saltos,pares_rede,perfis_influenciador` |
| `<prefixo>_perfis.csv` | `separacao` com `--output <prefixo>` | `perfil,alcancados,excentricidade,dist_media,dist_influenciador_mais_proximo,influenciadores_alcancados,dist_media_influenciadores` |
| `<pasta>/bfs_<semente>_n<N>.txt` | `--amostrar <pasta>` | edge list no formato de entrada, com cabeçalho de comentários |

No log, o `timestamp` está em UTC (ISO 8601), e o `algoritmo` é o nome de `--algo`,
exceto em `componentes`, que grava `componentes_fortes` e `componentes_fracos`. A pasta
de saída precisa existir: o programa não cria pastas.

## Dataset

**ego-Twitter**, da coleção SNAP de Stanford:
<https://snap.stanford.edu/data/ego-Twitter.html>

Cada linha `A B` significa que o usuário A **segue** o usuário B. O grafo é, portanto,
**direcionado**. O arquivo não é versionado (44 MB descompactado). Para obtê-lo:

```bash
./scripts/baixar_dataset.sh
```

O script baixa o arquivo para `data/raw/`, confere o SHA-256 e descompacta.

Números medidos no arquivo:

| Medida | Valor |
|---|---|
| Vértices | 81.306 |
| Arestas distintas | 1.768.135 |
| Linhas no arquivo | 2.420.766 (652.609 duplicadas e 22 laços) |
| Pares que se seguem mutuamente | 425.839 (48,2% das arestas são recíprocas) |
| Arestas na versão simetrizada | 1.342.296 |
| Maior número de seguidores | 3.383 |
| Usuários que não seguem ninguém | 11.211 |
| Maior ID original | 568.770.231 |

Três consequências para a implementação:

- **O arquivo repete arestas.** Ele é a união das redes pessoais de vários usuários, e
  a mesma relação aparece em mais de uma delas. O leitor precisa descartar duplicatas
  e laços, senão graus e contagens saem inflados.
- **Os IDs não servem como índice.** Vão até 568 milhões para apenas 81 mil vértices,
  por isso são remapeados para `0..V-1` por uma tabela hash.
- **A matriz de adjacência não cabe no grafo completo.** São 81 mil × 81 mil células:
  cerca de 6,6 GB a 1 byte por célula, ou 830 MB a 1 bit. A comparação lista × matriz é
  feita nos recortes amostrados, e o limite no grafo completo é tratado como resultado
  experimental.

Fonte para citação no artigo: J. McAuley e J. Leskovec. *Learning to Discover Social
Circles in Ego Networks*. NIPS, 2012.

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
