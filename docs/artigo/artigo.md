# Análise de Topologia e Conectividade na Disseminação de Informações em Redes Sociais

**João Pedro Gonçalves Campos Silvas, João Otavio Troiano Segoria, Gabriel Castagnaro Macêdo, Marcos Vinícius de Almeida Mendes**

Curso de Ciência da Computação – Instituto de Educação Superior de Brasília (IESB)
Brasília – DF – Brasil

## Resumo
Este artigo apresenta o desenvolvimento e a análise estrutural de uma aplicação baseada na Teoria dos Grafos para investigar o fluxo de informações na rede social Twitter. Utilizando uma abordagem puramente topológica na Fase I, o sistema modela usuários como vértices e relações de seguimento como arestas direcionadas. O software foi construído nativamente em linguagem C com foco em eficiência de baixo nível e alternância dinâmica de representações em tempo de execução. Testes preliminares validaram o uso de algoritmos estruturais e buscas iterativas para mapear fenômenos de mundo pequeno, ciclos de retroalimentação e vulnerabilidades de fragmentação na malha social.

## 1. Introdução e Modelagem do Problema
O entendimento de como dados e campanhas de informação se propagam em ambientes virtuais é um desafio crítico para a computação moderna. Redes estruturadas em larga escala, como o Twitter, apresentam comportamentos complexos que determinam se uma mensagem atingirá o público geral ou ficará retida em bolhas ideológicas.

Para investigar essa dinâmica, a rede social foi modelada como um grafo direcionado e não-valorado G = (V, E). Cada vértice v ∈ V representa de forma unívoca um usuário da plataforma. Uma aresta direcionada (u, v) ∈ E é instanciada se, e somente se, o usuário u segue o usuário v, estabelecendo o canal físico por onde a informação transita de v para u. Devido ao comportamento assimétrico da plataforma (onde seguir não implica reciprocidade), o direcionamento é fundamental na análise de propagação original.

## 2. Metodologia e Arquitetura do Sistema
Para atender aos requisitos de desempenho e consumo de memória (RF02 e RF03), o sistema foi projetado em C puro (padrão C11), sem o auxílio de bibliotecas externas para o core de grafos (RNF01). 

A arquitetura adota um design polimórfico baseado em uma tabela de operações abstrata (`GraphOps`). Esse desacoplamento permite ao sistema alternar dinamicamente em tempo de execução entre duas estruturas de dados fundamentais:
* **Lista de Adjacência:** Otimizada para grafos esparsos, economizando memória no heap.
* **Matriz de Adjacência:** Otimizada para consultas rápidas de adjacência O(1) à custa de um espaço quadrático \(O(\vert{}V\vert{}^2)\).

Para blindar o sistema contra falhas de estouro de pilha de execução (*Stack Overflow*) ao processar bases com milhares de vértices, os algoritmos estruturais — como a Busca em Profundidade (DFS) e a identificação de componentes conexos — foram implementados utilizando pilhas explícitas alocadas dinamicamente na memória (*heap*). A instrumentação temporal foi construída com a chamada de sistema `clock_gettime(CLOCK_MONOTONIC)`, garantindo precisão em milissegundos nas medições do protocolo experimental.

Todas as execuções do protocolo experimental foram feitas em uma única máquina, descrita na Tabela 1, em uma mesma sessão e sem outros programas pesados abertos.

**Tabela 1.** Especificação da máquina de testes.

| Item | Especificação |
|---|---|
| Processador | Intel Core i7-8550U (8ª geração) @ 1.80GHz |
| Núcleos / threads | 4 núcleos, 8 threads |
| Memória RAM | 16 GB DDR4 2400 MT/s |
| Sistema operacional | Bluefin (Fedora Atômico), kernel 6.16.8-200.fc42.x86_64 — compilação via toolbox Ubuntu 24.04.4 LTS |
| Compilador | GCC 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1) |
| Opções de compilação | `-std=c11 -O2 -Wall -Wextra -pedantic -Werror -D_POSIX_C_SOURCE=200809L` |
| Data da coleta | 01/10/2026 |

## 3. Resultados

Os seis algoritmos da Fase I foram executados sobre as 12 amostras (N = 100, 250, 500 e 1.000 vértices, três sementes por tamanho) e sobre o grafo completo do ego-Twitter (|V| = 81.306, |E| = 1.768.135), nas duas representações. Cada configuração rodou 11 vezes seguidas, e a primeira execução, de aquecimento, foi descartada. As 2.090 execuções estão em `results/log.csv`, as médias e desvios padrão em `results/resumo.csv` e a máquina usada em `results/maquina.txt` (Intel Core i7-8550U, 15 GB de RAM, GCC 13.3 com `-O2`). Nenhuma configuração falhou (`results/inviaveis.csv` está vazio): a matriz coube na memória mesmo no grafo completo, o que só foi possível por ela ser uma matriz de bits.

### 3.1 Tempo de execução

A Figura 1 (`results/figs/tempo.png`) mostra o tempo de cada algoritmo em função de |V| + |E|, em escala log-log, contra a reta de referência O(V + E). Com lista de adjacência, todos os algoritmos acompanham a reta, do menor grafo (≈ 1.200 elementos) ao maior (≈ 1,8 milhão): a BFS leva 0,10 ms em N = 1.000 e 11,6 ms no grafo completo, e a articulação, a mais cara, 47,3 ms.

Com a matriz, a curva se afasta da reta à medida que o grafo cresce. Percorrer os vizinhos de um vértice na matriz exige examinar a linha inteira, e a implementação pula palavras de 64 bits vazias; o custo total passa a ser O(V²/64 + E). Nas amostras, o termo V²/64 (no máximo 15.625 palavras, em N = 1.000) é da mesma ordem que |E|, e a matriz fica 1,4 a 1,9 vez mais lenta que a lista. No grafo completo, V²/64 chega a 103 milhões de palavras, contra 1,77 milhão de arestas, e domina o custo. A Tabela 2 resume a diferença.

**Tabela 2.** Tempo médio no grafo completo (ms).

| Algoritmo | Lista | Matriz | Matriz / Lista |
|---|---|---|---|
| BFS | 11,6 | 107,8 | 9,3 |
| DFS | 21,8 | 167,4 | 7,7 |
| Ciclos | 22,1 | 168,0 | 7,6 |
| Componentes fortes | 26,2 | 170,4 | 6,5 |
| Componentes fracos | 39,2 | 167,3 | 4,3 |
| Bipartição | 36,9 | 155,5 | 4,2 |
| Pontes e articulação | 47,3 | 168,8 | 3,6 |

Na matriz, o tempo é praticamente o mesmo para todos os algoritmos (155 a 170 ms, exceto a BFS), porque todos visitam cada vértice uma vez e varrem a sua linha inteira: a varredura das V²/64 palavras é o custo comum, e o trabalho próprio de cada algoritmo fica escondido sob ela. Por isso a razão é maior nos algoritmos mais leves e menor nos mais pesados. A BFS é a exceção porque só varre as linhas dos vértices que alcança a partir da origem. Seu desvio padrão alto (7,99 ms na lista, 78,1 ms na matriz) também não é ruído de medição: no grafo completo ela parte de três sementes diferentes, que alcançam partes diferentes da rede.

### 3.2 Consumo de memória

A Figura 2 (`results/figs/memoria.png`) compara a memória das duas representações. O resultado se inverte entre as amostras e o grafo completo:

- **Nas amostras, a matriz ocupa menos memória que a lista**, de 0,35 a 0,51 vez, conforme o algoritmo. Em N = 1.000, a BFS usa 376 KB com lista e 181 KB com matriz.
- **No grafo completo, a matriz ocupa cerca de 800 MB** (813.035 KB na BFS), contra 26,6 MB da lista: de 18 a 31 vezes mais, conforme o algoritmo, e 24 vezes na média.

A memória da matriz tem desvio padrão nulo em todos os tamanhos, porque depende só de N: as três amostras de mesmo tamanho alocam exatamente a mesma matriz, mesmo tendo números diferentes de arestas. A da lista varia entre as amostras porque acompanha |E|.

## 4. Discussão

### 4.1 Por que a matriz é inviável em redes sociais esparsas

A inversão observada na memória se explica pela densidade d = |E| / |V|², e não pelo tamanho do grafo. A matriz de bits ocupa V²/8 bytes, qualquer que seja o número de arestas. Descontados os vetores auxiliares dos algoritmos, que são O(V) e iguais nas duas representações, a lista ocupa cerca de 12 bytes por aresta nas medições (11,9 B em N = 1.000 e 12,1 B no grafo completo). As duas se igualam quando

V² / 8 = 12 · |E|, ou seja, d = 1/96 ≈ 1%.

Esse limiar descreve os dois extremos medidos:

- As amostras são recortes por BFS em torno de uma semente e, por isso, densas: em N = 1.000 há em média 27.526 arestas, d ≈ 2,8%. O modelo prevê que a estrutura da matriz ocupe 1,04 / 2,75 ≈ 0,38 vez a da lista; o medido é 125 KB / 320 KB = 0,39.
- O grafo completo tem d = 1.768.135 / 81.306² ≈ 0,027%, quase 40 vezes abaixo do limiar. O modelo prevê uma matriz 39 vezes maior que a lista; o medido, só para a estrutura, é 807 MB / 20,9 MB = 38,5.

A memória O(V²) da matriz, portanto, não é inviável por si só, mas é inviável **para redes sociais**, porque elas são esparsas e ficam mais esparsas à medida que crescem. O número de perfis que um usuário consegue acompanhar é limitado e cresce muito mais devagar que a rede: no ego-Twitter, cada usuário segue em média 21,7 outros, de 81.306 possíveis. Se |E| cresce aproximadamente como O(V) e a matriz como O(V²), a razão entre elas cresce linearmente com V. O ego-Twitter já está no limite do que uma máquina comum suporta: os 813 MB só couberam por usarmos um bit por célula, e com um byte por célula seriam 6,6 GB. Na escala da plataforma real, uma matriz de bits para 3 × 10⁸ usuários ocuparia mais de 11 PB, enquanto a lista continuaria proporcional às relações existentes.

O tempo segue o mesmo raciocínio. Os algoritmos da Fase I são todos travessias, que perguntam "quais são os vizinhos de v?", e a matriz responde a essa pergunta em O(V) em vez de O(grau(v)). Mesmo com o salto de palavras vazias, que divide o custo por 64, a varredura de linhas quase vazias passa a dominar quando o grau médio é uma fração ínfima de V, e a matriz fica de 3,6 a 9,3 vezes mais lenta. No grafo completo, a matriz piora ao mesmo tempo a memória e o tempo; foi por isso que a análise de graus de separação, com 1.000 buscas em largura, foi feita só com a lista (`results/separacao.md`).

### 4.2 Onde a matriz ainda compensa

A matriz não é uma escolha errada em geral; ela é errada para o padrão de acesso e a densidade deste problema. Ela compensa em três situações:

1. **Consultas de adjacência.** A operação `graph_has_edge(u, v)` ("u segue v?") custa O(1) na matriz, com um único acesso a bit, e O(grau(u)) na lista. Algoritmos dominados por esse tipo de consulta, e não por travessias, invertem a vantagem. O caso mais próximo neste projeto é a Clique Máxima da Fase II, sobre a visão recíproca: verificar se um conjunto candidato é uma clique exige testar a adjacência de todos os seus pares. Os experimentos desta fase medem só travessias, então essa vantagem é prevista pela complexidade, e não medida.
2. **Subgrafos densos.** Acima de d ≈ 1%, a matriz de bits ocupa menos memória que a lista, como mostraram as amostras. Recortes locais da rede, como a vizinhança de um usuário ou uma comunidade, são densos o bastante para isso, e neles a diferença de tempo também é pequena (1,4 a 1,9 vez em N = 1.000).
3. **Grafos pequenos.** Com alguns milhares de vértices, a matriz cabe com folga na memória (10.000 vértices dão 12,5 MB) e o termo V²/64 não chega a dominar. Nesse regime, a simplicidade da matriz e o acesso O(1) podem valer mais que a economia da lista.

Uma estratégia para a Fase II é combinar as duas: manter o grafo completo em lista de adjacência e construir matrizes de bits só para os subgrafos densos sobre os quais forem feitas muitas consultas de adjacência. A interface única `GraphOps` permite essa troca sem alterar os algoritmos.

### 4.3 Limitações

As amostras foram obtidas por BFS a partir de sementes e, por isso, são mais densas que a rede da qual vieram; elas não representam o grafo completo em escala reduzida, e é justamente esse contraste que expõe o ponto de virada. As medições foram feitas em uma única máquina, e os tempos absolutos dependem dela; o que se transfere para outras máquinas são as razões entre lista e matriz e as tendências de crescimento. Por fim, a memória medida é a alocada pelo programa, via `memtrack`, e não a memória residente do processo.
