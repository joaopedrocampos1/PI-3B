# Graus de separação: perfis comuns e influenciadores (#17)

Pergunta do estudo de caso: **qual o grau de separação médio entre perfis comuns e
grandes influenciadores desta rede?**

Resultados gerados por:

```bash
./bin/grafos --input data/raw/twitter_combined.txt --algo separacao --output results/separacao
```

O comando grava `separacao_histograma.csv` e `separacao_perfis.csv` nesta pasta. A
semente padrão (`--semente-rng 2026`) reproduz exatamente estes arquivos.

## Método

### Direção

Na visão direcionada, "A segue B" é a aresta A → B, e a informação anda no sentido
contrário: as publicações de B aparecem para A. O BFS parte de cada perfil comum P e
**segue o sentido das arestas**. Assim, a distância de P até um influenciador I é o
número de repasses necessários para uma publicação de I chegar até P.

### Critério de influenciador

**Influenciadores são o 1% de usuários com mais seguidores** (maior grau de entrada na
visão direcionada). Os empatados com o último da lista também entram. No grafo
completo, isso dá **817 influenciadores, todos com pelo menos 209 seguidores**.

Justificativa:

- **Corte relativo, e não um número absoluto de seguidores.** O ego-Twitter é a união
  das redes pessoais de alguns usuários. O número de seguidores de cada um *dentro da
  base* não é o número real que ele tem no Twitter, então um corte absoluto ("mais de
  10 mil seguidores") não teria significado aqui.
- **O mesmo critério vale para qualquer tamanho.** Um percentil se aplica igualmente
  às amostras de N = 100 a 1.000 e ao grafo completo, o que permite comparar os
  resultados entre elas.
- **1% isola o topo da distribuição.** Esse grupo é pequeno o bastante para conter só
  os usuários de maior audiência e grande o bastante (817) para que a distância "até o
  influenciador mais próximo" não dependa de um único vértice.
- **Os empates entram.** Com eles, o conjunto não depende da ordem dos índices, e o
  resultado é o mesmo com lista e matriz.

O resultado depende desse corte. Com mais influenciadores, a distância até o mais
próximo tende a cair. Esta justificativa deve ir para o artigo (#31).

### Perfis comuns

São sorteados **1.000 perfis comuns**, de modo uniforme e sem reposição, entre os
vértices que não são influenciadores. O sorteio usa o gerador splitmix64 com a semente
2026 e é reprodutível em qualquer plataforma.

### Métricas

Um BFS a partir de cada perfil sorteado fornece:

- a distância até o influenciador **mais próximo**;
- a distância média até **todos** os influenciadores alcançados ("o conjunto como um
  todo");
- a distância média até **todos** os vértices alcançados, que serve de estimativa da
  distância média geral da rede;
- a excentricidade de cada perfil. A maior delas é uma estimativa do diâmetro,
  considerando só os pares alcançáveis, já que o grafo direcionado não é fortemente
  conexo.

## Resultados no grafo completo

| Medida | Valor |
|---|---|
| Influenciadores | 817 (≥ 209 seguidores) |
| Perfis comuns analisados | 1.000 |
| **Distância média perfil comum → influenciador mais próximo** | **1,38 saltos** |
| Distância média perfil comum → todos os influenciadores alcançados | 3,60 saltos |
| **Distância média geral da rede** (a partir dos perfis) | **4,87 saltos** |
| Perfis que não alcançam nenhum influenciador | 184 (18,4%) |
| Diâmetro estimado (maior excentricidade observada) | 11 |

Histograma (`separacao_histograma.csv`):

| Saltos | Perfis cujo influenciador mais próximo está a essa distância | Pares (perfil, vértice) da rede |
|---|---|---|
| 1 | 558 | 20.509 |
| 2 | 216 | 537.481 |
| 3 | 36 | 5.793.187 |
| 4 | 5 | 19.132.733 |
| 5 | 1 | 22.856.766 |
| 6 | 0 | 12.824.795 |
| 7 | 0 | 4.031.921 |
| 8 | 0 | 785.218 |
| 9 | 0 | 108.770 |
| 10 | 0 | 7.879 |
| 11 | 0 | 184 |

Leitura:

- **Influenciadores estão muito mais perto que o resto da rede.** Entre os perfis que
  alcançam algum influenciador, a distância média até o mais próximo é 1,38 saltos,
  contra 4,87 até um usuário qualquer. Mais da metade dos perfis (558 de 1.000) segue
  diretamente um influenciador, e 774 estão a no máximo 2 saltos.
- **Quem alcança um influenciador alcança quase a rede inteira.** Dos 816 perfis nessa
  situação, 813 alcançam pelo menos 81.000 dos 81.306 vértices.
- **Os 18,4% sem alcance são, na maioria, perfis que não seguem ninguém.** São 164 dos
  184 casos. Os outros 20 seguem apenas grupos pequenos, de 2 a 63 usuários, sem
  nenhum influenciador.

## Validação nas amostras: estimativa × valor exato

Nas amostras, que têm até 1.000 vértices, o programa também calcula o diâmetro e a
distância média **exatos**, com um BFS a partir de cada vértice. A estimativa feita a
partir dos perfis sorteados coincide com o valor exato no diâmetro e fica a menos de
0,005 na distância média. Por exemplo, na amostra `bfs_144319796_n1000` a estimativa
dá diâmetro 10 e média 3,3169, e o valor exato é diâmetro 10 e média 3,3147.

| Amostra | Diâmetro exato | Distância média exata | Perfil → influenciador mais próximo | Perfis sem influenciador |
|---|---|---|---|---|
| bfs_144319796_n100 | 4 | 1,9106 | 1,3261 | 7,07% |
| bfs_307642294_n100 | 4 | 1,8752 | 1,2268 | 2,02% |
| bfs_9487272_n100 | 5 | 2,3370 | 1,2619 | 13,40% |
| bfs_144319796_n250 | 7 | 2,5561 | 1,4779 | 8,50% |
| bfs_307642294_n250 | 6 | 2,3571 | 1,0367 | 0,81% |
| bfs_9487272_n250 | 7 | 2,9621 | 1,3917 | 12,15% |
| bfs_144319796_n500 | 8 | 3,1235 | 1,6261 | 5,45% |
| bfs_307642294_n500 | 5 | 2,5189 | 1,0203 | 0,40% |
| bfs_9487272_n500 | 8 | 2,9795 | 1,2376 | 6,46% |
| bfs_144319796_n1000 | 10 | 3,3147 | 1,5766 | 3,13% |
| bfs_307642294_n1000 | 5 | 2,5516 | 1,0587 | 0,20% |
| bfs_9487272_n1000 | 8 | 2,9965 | 1,2282 | 4,75% |

Lista e matriz de adjacência produzem saídas idênticas nas amostras. No grafo
completo, a análise foi feita só com a lista: com a matriz de bits, cada BFS custa
O(V²/64), o que torna inviável repeti-lo para 1.000 perfis.
