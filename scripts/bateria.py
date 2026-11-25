#!/usr/bin/env python3
"""Bateria de testes de estresse (#27), seguindo o protocolo da Metodologia do
artigo (#26):

  13 instâncias (as 12 amostras de data/samples e o recorte de 10 mil
  vértices, data/recorte_n10000.txt, que substitui o grafo completo: #59)
  x 2 representações (lista e matriz)
  x 6 algoritmos (bfs, dfs, componentes, ciclos, bipartido, articulacao),
  cada configuração executada 11 vezes seguidas; a 1ª é o aquecimento.

BFS e DFS partem da semente da própria instância, lida do cabeçalho do
arquivo ("# semente: ..."); o recorte é tratado como as amostras, o que dá
156 configurações. Cada execução acrescenta uma linha a results/log.csv
(componentes acrescenta duas: fortes e fracos). Uma configuração que falha,
como a matriz sem memória, é registrada em results/inviaveis.csv e a bateria
segue: o protocolo trata esse limite como resultado.

Uso, a partir de qualquer pasta:
  python3 scripts/bateria.py

Para um teste rápido, sem seguir o protocolo inteiro, as mesmas variáveis de
ambiente do antigo bateria.sh (INSTANCIAS separadas por espaço):
  REPETICOES=2 LOG=bin/log.csv INVIAVEIS=bin/inviaveis.csv \\
  INSTANCIAS="data/samples/bfs_9487272_n100.txt" python3 scripts/bateria.py
No PowerShell, defina antes com $env:REPETICOES = "2" etc.
"""

import glob
import os
import re
import subprocess
import sys
import time

from comum import GRAFOS, compilar, ir_para_raiz
from maquina import descrever_maquina

RECORTE = "data/recorte_n10000.txt"
ALGORITMOS = ["bfs", "dfs", "componentes", "ciclos", "bipartido", "articulacao"]
ESTRUTURAS = ["lista", "matriz"]

REPETICOES = int(os.environ.get("REPETICOES", "11"))
LOG = os.environ.get("LOG", "results/log.csv")
INVIAVEIS = os.environ.get("INVIAVEIS", "results/inviaveis.csv")
MAQUINA = os.environ.get("MAQUINA", "results/maquina.txt")
ERROS = os.path.join("bin", "bateria_erros.txt")


def instancias():
    if os.environ.get("INSTANCIAS"):
        return os.environ["INSTANCIAS"].split()
    return sorted(glob.glob("data/samples/bfs_*.txt")) + [RECORTE]


def origens_de(instancia):
    """A semente de uma instância está no cabeçalho gravado por --amostrar
    e --recorte ("# semente: <id>"); nas amostras, também no nome,
    bfs_<semente>_n<N>.txt. Sem nenhum dos dois, a bateria para: BFS e DFS
    não teriam de onde partir."""
    with open(instancia, encoding="utf-8", errors="replace") as f:
        for linha in f:
            if not linha.startswith("#"):
                break
            m = re.match(r"#\s*semente:\s*(\d+)", linha)
            if m:
                return [m.group(1)]
    m = re.match(r"bfs_(\d+)_n", os.path.basename(instancia))
    if not m:
        sys.exit(f"erro: {instancia} não informa a semente (cabeçalho \"# semente: <id>\")")
    return [m.group(1)]


def configuracoes(lista):
    """Uma tupla por configuração: (instância, estrutura, algoritmo, origem)."""
    for inst in lista:
        for est in ESTRUTURAS:
            for algo in ALGORITMOS:
                if algo in ("bfs", "dfs"):
                    for o in origens_de(inst):
                        yield inst, est, algo, o
                else:
                    yield inst, est, algo, "-"


def main():
    ir_para_raiz()
    lista = instancias()

    if os.path.isfile(LOG) and os.path.getsize(LOG) > 0:
        sys.exit(f"erro: {LOG} já existe. A bateria grava um log limpo; mova ou apague o atual antes.")
    for f in lista:
        if not os.path.isfile(f):
            sys.exit(f"erro: {f} não existe")
    compilar()

    # Especificação da máquina, exigida pelo protocolo.
    with open(MAQUINA, "w", encoding="utf-8", newline="\n") as f:
        f.write(descrever_maquina(REPETICOES))

    if not (os.path.isfile(INVIAVEIS) and os.path.getsize(INVIAVEIS) > 0):
        with open(INVIAVEIS, "w", encoding="utf-8", newline="\n") as f:
            f.write("dataset,estrutura,algoritmo,origem,codigo\n")

    confs = list(configuracoes(lista))
    total = len(confs)
    inicio = time.monotonic()
    print(f"bateria: {total} configurações x {REPETICOES} execuções, log em {LOG}", flush=True)

    with open(ERROS, "a", encoding="utf-8") as erros:
        for feitas, (inst, est, algo, origem) in enumerate(confs, 1):
            args = [GRAFOS, "--input", inst, "--struct", est, "--algo", algo, "--log", LOG]
            if origem != "-":
                args += ["--source", origem]

            for execucao in range(1, REPETICOES + 1):
                codigo = subprocess.run(args + ["--execucao", str(execucao)],
                                        stdout=subprocess.DEVNULL, stderr=erros).returncode
                if codigo != 0:
                    with open(INVIAVEIS, "a", encoding="utf-8", newline="\n") as f:
                        f.write(f"{inst},{est},{algo},{origem},{codigo}\n")
                    print(f"  inviável: {inst} {est} {algo} (código {codigo})")
                    break

            decorrido = int(time.monotonic() - inicio)
            nome = os.path.basename(inst)
            print(f"[{feitas:3d}/{total}] {nome:<42} {est:<6} {algo:<12} {decorrido:5d}s", flush=True)

    with open(LOG, encoding="utf-8") as f:
        linhas = sum(1 for _ in f) - 1
    print(f"bateria concluída em {int(time.monotonic() - inicio)} s: {linhas} linhas em {LOG}")


if __name__ == "__main__":
    main()
