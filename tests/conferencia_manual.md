# Conferência manual dos algoritmos (#13, antigo T22)

Validação cruzada: dois grafos pequenos, desenhados à mão, com a resposta de cada
algoritmo da Fase I calculada no papel. O programa precisa chegar exatamente aos
mesmos resultados, nas duas representações.

Os grafos estão em `tests/grafos/` no formato de edge list, e os IDs coincidem com os
índices internos (0..V-1), porque aparecem em ordem crescente no arquivo.

A conferência de cada algoritmo é automatizada em `tests/test_conferencia.c` assim
que ele é implementado. Quem implementar um algoritmo da tabela de status acrescenta
o caso correspondente nesse arquivo, usando o gabarito abaixo.

## Status

| Algoritmo | Issue | Conferência |
|---|---|---|
| BFS | #14 | ✅ automatizada em `test_conferencia.c` (G1 e G2, lista e matriz) |
| DFS iterativa | #15 | ✅ automatizada em `test_conferencia.c` (G1, lista e matriz; o gabarito não traz DFS para o G2) |
| Componentes conexos | #18 | ✅ automatizada em `test_conferencia.c` (G1 e G2, lista e matriz) |
| Detecção de ciclos | #19 | ✅ automatizada em `test_conferencia.c` (G1 e G2, lista e matriz) |
| Bipartição | #20 | ✅ automatizada em `test_conferencia.c` (G1 e G2, lista e matriz) |
| Pontes e articulação (Tarjan) | #21 | ✅ automatizada em `test_conferencia.c` (G1 e G2, lista e matriz) |

## G1: 12 vértices, 12 arestas (`conferencia_g1.txt`)

Visão direcionada (A → B: A segue B):

```
   0 ──→ 1 ──→ 2 ──→ 3 ──→ 4 ──→ 5 ──→ 6 ──→ 7 ──→ 8
   ↑           │     ↑           │
   └───────────┘     └───────────┘
      2 → 0             5 → 3

   9 ──→ 10 ──→ 11
```

Arestas: 0→1, 1→2, 2→0, 2→3, 3→4, 4→5, 5→3, 5→6, 6→7, 7→8, 9→10, 10→11.
Nenhum par mútuo, então a visão simetrizada tem as mesmas 12 arestas, sem sentido.

### BFS, visão direcionada

| Origem | Distâncias (vértice: saltos) | Alcançados | Excentricidade | Distância média |
|---|---|---|---|---|
| 0 | 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8; 9, 10, 11 inalcançáveis | 9 | 8 | 36/8 = 4,5 |
| 2 | 0:1, 3:1, 1:2, 4:2, 5:3, 6:4, 7:5, 8:6 | 9 | 6 | 24/8 = 3,0 |
| 6 | 7:1, 8:2 | 3 | 2 | 1,5 |
| 8 | ninguém além dele (não segue ninguém) | 1 | 0 | 0 |
| 9 | 10:1, 11:2 | 3 | 2 | 1,5 |

Caminho mínimo de 0 até 8: 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 (8 saltos).

### BFS, visão simetrizada, a partir de 0

| Vértice | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9–11 |
|---|---|---|---|---|---|---|---|---|---|
| Distância | 1 | 1 | 2 | 3 | 3 | 4 | 5 | 6 | ∞ |
| Predecessor | 0 | 0 | 2 | 3 | 3 | 5 | 6 | 7 | — |

Alcançados: 9. Excentricidade: 6. Distância média: 25/8 = 3,125.
Caminho de 0 até 8: 0 → 2 → 3 → 5 → 6 → 7 → 8 (6 saltos).

### DFS iterativa, visão direcionada, a partir de 0

Com vizinhos em ordem crescente, a ordem de descoberta é 0, 1, 2, 3, 4, 5, 6, 7, 8.
Vértices 9, 10 e 11 não são alcançados a partir de 0.

### Componentes conexos, visão simetrizada

2 componentes: {0, 1, 2, 3, 4, 5, 6, 7, 8} (9 vértices) e {9, 10, 11} (3 vértices).

### Detecção de ciclos, visão direcionada

Há ciclos: 0 → 1 → 2 → 0 e 3 → 4 → 5 → 3. Na DFS a partir de 0, com vizinhos em
ordem crescente, as arestas de retorno são 2 → 0 e 5 → 3. O componente {9, 10, 11}
não tem ciclos.

### Bipartição, visão simetrizada

**Não é bipartido.** Ciclos ímpares: o triângulo 0–1–2 e o triângulo 3–4–5.
O componente {9, 10, 11}, sozinho, seria bipartido ({9, 11} e {10}).

### Pontes e vértices de articulação, visão simetrizada

- Pontes (6): {2, 3}, {5, 6}, {6, 7}, {7, 8}, {9, 10}, {10, 11}.
- Vértices de articulação (6): 2, 3, 5, 6, 7, 10.

Conferência: as arestas dos triângulos 0–1–2 e 3–4–5 não são pontes, porque cada uma
está em um ciclo. O vértice 4 não é articulação: sem ele, 3 e 5 continuam ligados.

## G2: 5 vértices, 5 arestas (`conferencia_g2.txt`)

```
   0 ──→ 1
   ↑     │
   │     ↓
   3 ←── 2
   │
   ↓
   4
```

Arestas: 0→1, 1→2, 2→3, 3→0, 3→4.

### BFS

- Direcionada, a partir de 0: 1:1, 2:2, 3:3, 4:4. Excentricidade 4.
- Simetrizada, a partir de 0: 1:1, 3:1, 2:2, 4:2. Excentricidade 2.

### Demais algoritmos

- **Componentes** (simetrizada): 1 componente com os 5 vértices.
- **Ciclos** (direcionada): há o ciclo 0 → 1 → 2 → 3 → 0; aresta de retorno 3 → 0.
- **Bipartição** (simetrizada): **bipartido**, lados {0, 2, 4} e {1, 3}.
- **Tarjan** (simetrizada): ponte {3, 4}; vértice de articulação 3.
