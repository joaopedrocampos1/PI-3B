# Modelo de custo: engajamento e confiança por vizinhos em comum (#65)

Texto de apoio para a Metodologia do artigo (Fase II). A implementação está em
`include/custo.h` e `src/custo.c`; os testes, em `tests/test_custo.c`.

## O problema

O Dijkstra da Fase II (#66) precisa de arestas com peso, mas o ego-Twitter não traz
engajamento, curtidas nem confiança: cada linha é só "A segue B". O enunciado permite
derivar o peso da **similaridade** entre os usuários. Usamos a estrutura da própria rede.

## O modelo

```
custo(A -> B) = 1 / (1 + c)
c = |N(A) ∩ N(B)|, mais 1 se A e B se seguem mutuamente
```

- **N(x)** é o círculo de x: os vizinhos de x na visão simetrizada, ou seja, quem x
  segue e quem segue x. Como o grafo não tem laços, A e B não entram na interseção.
- **Contatos em comum** são a medida clássica de similaridade estrutural (é a base de
  índices como Jaccard e Adamic–Adar). Dois usuários com muitos contatos em comum estão
  no mesmo círculo social, e uma publicação de um tende a ser bem recebida pelo outro.
- **A reciprocidade** soma 1: quem se segue mutuamente tem um laço mais forte do que um
  seguidor de mão única.
- **O custo fica em (0, 1].** Vale 1 quando não há nenhum contato em comum (atrito
  máximo) e diminui conforme a confiança aumenta. O denominador nunca é zero.
- **É simétrico:** custo(A -> B) = custo(B -> A) quando as duas arestas existem.

## Sentido da propagação

"A segue B" é a aresta A -> B da visão direcionada, mas a **publicação anda de B para A**
(a mesma convenção da #17). Por isso o grafo com peso é montado, por padrão, no sentido
da propagação: cada "A segue B" vira a aresta **B -> A** com o custo como peso
(`CUSTO_SENTIDO_PROPAGACAO`). Um Dijkstra a partir de quem publica segue as arestas
desse grafo e devolve o caminho de menor atrito da campanha. O sentido das arestas
originais continua disponível (`CUSTO_SENTIDO_SEGUE`). Como o custo é simétrico, os
dois grafos têm os mesmos pesos e só diferem no sentido das arestas.

## Custo de cálculo

Para cada aresta, os vizinhos do vértice de menor grau são consultados no círculo do
outro: O(Σ min(grau(A), grau(B))) consultas, cada uma O(log grau) com a lista e O(1)
com a matriz. No recorte de 10 mil vértices (376.035 arestas), medido numa máquina com
Intel Xeon 2,1 GHz, gcc -O2:

| Representação | Tempo para calcular os custos e montar o grafo com peso | Memória do grafo com peso |
|---|---|---|
| Lista | ~1,15 s | 6,9 MB |
| Matriz | ~0,5 s | 412,7 MB |

A matriz consulta cada vizinho em O(1) e por isso calcula mais rápido, mas guarda um
`float` por par de vértices (#64): 60 vezes a memória da lista.

## Distribuição no recorte de 10 mil vértices

| Medida | Valor |
|---|---|
| Arestas | 376.035 |
| Arestas mútuas (A e B se seguem) | 183.726 (48,9%) |
| Custo 1 (nenhum contato em comum, sem reciprocidade) | 311 (0,1%) |
| c ≤ 4 (custo ≥ 0,2) | 16.463 (4,4%) |
| c ≤ 9 (custo ≥ 0,1) | 51.835 (13,8%) |
| Mediana de c | 28 (custo ≈ 0,034) |
| Média de c | 36,8 |
| Percentis 10, 90 e 99 de c | 8, 75 e 164 |
| Maior c | 872 |
| Custo médio | 0,055 |

O recorte é o subgrafo induzido pelos 10 mil primeiros vértices de um BFS, então é uma
região densa da rede: quase toda aresta liga usuários com dezenas de contatos em comum,
e só 0,1% das arestas têm o atrito máximo. A cauda é longa: 95% das arestas têm c ≤ 99,
mas alguns pares compartilham centenas de contatos (c = 872 no máximo), com custo perto
de 0,001. Na prática, o Dijkstra vai preferir passar por dentro dos círculos densos e
evitar as poucas arestas "frias", sem contatos em comum.

## Conferência

- Custos calculados à mão num grafo de 5 vértices (todas as arestas, nos dois sentidos).
- Lista e matriz dão pesos idênticos em grafos aleatórios.
- No recorte, os 376.035 custos e a distribuição conferem com uma implementação
  independente em C, com listas próprias e interseção por intercalação, sem passar pelo
  TAD, e com outra em Python, com conjuntos.
