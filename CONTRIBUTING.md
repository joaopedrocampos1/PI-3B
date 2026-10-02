# Convenções de trabalho

Regras combinadas pelo time para o PI-3B. Vale para todo mundo, inclusive para quem escreveu.

## Branches

- `main`: a única branch de integração. Ninguém commita direto: tudo entra por PR.
- `feature/<issue>-<descricao>`: uma branch por issue, criada a partir da `main`
  atualizada. Exemplo: `feature/14-bfs`, `feature/24-memtrack`.
- Depois do merge, a branch da issue é apagada.

## Commits

Seguimos [Conventional Commits](https://www.conventionalcommits.org/pt-br/):

| Prefixo | Quando usar |
|---|---|
| `feat:` | funcionalidade nova |
| `fix:` | correção de bug |
| `test:` | testes |
| `docs:` | README, artigo, comentários |
| `refactor:` | mudança de código sem mudar comportamento |
| `chore:` | Makefile, `.gitignore`, organização |

Mensagem no imperativo, em português, citando a issue:

```
feat: implementa BFS com reconstrução de caminho (#14)
fix: corrige contagem de arestas em grafo não direcionado (#12)
```

Evite commits como "ajustes", "final" ou "agora vai". Um commit, uma mudança.

## Pull requests

- Todo PR vai para a `main` e precisa de **pelo menos 1 aprovação** de outra pessoa.
- Mudanças em `include/graph.h` precisam de aprovação do **DEV3 e do DEV4**: é a
  interface que todos usam.
- Na descrição, use `Closes #N` para a issue fechar sozinha no merge.
- Quem abriu o PR faz o merge depois da aprovação.

## Regras de código

- **C11**, compilando **sem nenhum aviso** com `-std=c11 -Wall -Wextra -pedantic`.
- **Sem vazamentos**: rodar `valgrind --leak-check=full` antes de abrir o PR.
- **Nenhuma biblioteca de grafos** nos algoritmos (RNF01). Python e gnuplot só em `scripts/`.
- **Memória só pelo memtrack**: use `mt_malloc`, `mt_calloc`, `mt_realloc` e `mt_free`
  (`include/memtrack.h`) no lugar de `malloc`, `calloc`, `realloc` e `free`. Sem isso, a
  memória não entra na medição do RF03 e a comparação lista × matriz fica errada.
- **Algoritmos só incluem `graph.h`**, nunca `graph_internal.h`. Vizinhos se percorrem
  com `graph_neighbors_begin` / `graph_neighbors_next`, que funcionam nas duas
  representações.
- **DFS sempre iterativa**, com pilha explícita.
- Nomes em `snake_case`, com o módulo como prefixo (`graph_`, `mt_`, `log_`, `bfs_`).
