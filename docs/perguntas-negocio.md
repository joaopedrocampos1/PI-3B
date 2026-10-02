# T05 — Perguntas de Negócio da Fase I (Disseminação de Informação)

### P1: O Fenômeno do "Mundo Pequeno"
* **Pergunta de Negócio:** Qual é o grau de separação médio entre dois usuários quaisquer na rede social?
* **Abstração em Grafos:** Caminho mínimo médio calculado via BFS em todas as combinações de vértices.

### P2: O Alcance Máximo da Disseminação
* **Pergunta de Negócio:** A partir de um usuário específico que inicia uma campanha, qual é o número máximo de saltos necessários para que a informação atinja o limite da rede, e qual a porcentagem de usuários fica completamente isolada?
* **Abstração em Grafos:** Determinar a excentricidade do vértice de origem via BFS e o tamanho do componente gigante direcionado.

### P3: Câmaras de Eco e Retroalimentação
* **Pergunta de Negócio:** Existem caminhos fechados na rede que fazem com que uma informação compartilhada retorne ao seu gerador original após passar por outros usuários?
* **Abstração em Grafos:** Identificação de ciclos direcionados e classificação de arestas de retorno utilizando a DFS iterativa.

### P4: Polarização e Bolhas Ideológicas
* **Pergunta de Negócio:** A estrutura da rede permite dividir os usuários em dois grupos completamente distintos (ex: polos políticos) onde as interações ocorrem apenas entre grupos opostos, e nunca internamente?
* **Abstração em Grafos:** Aplicação do teste de Bipartição de Grafos (coloração com 2 cores usando BFS/DFS na visão simetrizada).

### P5: Vulnerabilidade da Rede e Usuários-Chave
* **Pergunta de Negócio:** Quais usuários ou conexões funcionam como pontes críticas de informação, cuja remoção causaria a fragmentação da rede social em subcomunidades incapazes de se comunicarem?
* **Abstração em Grafos:** Identificação de vértices de articulação e pontes através do algoritmo de Tarjan na visão simetrizada.
  
