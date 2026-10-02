# Análise de Topologia e Conectividade na Disseminação de Informações em Redes Sociais

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
