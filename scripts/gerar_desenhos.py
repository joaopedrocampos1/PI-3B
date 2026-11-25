#!/usr/bin/env python3
"""Gera os desenhos versionados em results/dot/ (.dot e .png): os seis
algoritmos sobre os grafos da conferência manual (G1 e G2, a partir do
vértice 0) e sobre uma amostra de 100 vértices (a partir da sua semente).

Os .dot saem do programa (--dot); os PNG, de scripts/render_dot.py, que
requer o Graphviz. As execuções vão para um log descartável, para não
misturar com results/log.csv da bateria.

Uso, a partir de qualquer pasta:
  python3 scripts/gerar_desenhos.py
"""

import contextlib
import glob
import io
import os
import subprocess
import sys

from comum import GRAFOS, compilar, ir_para_raiz
from render_dot import renderizar

ALGORITMOS = ["bfs", "dfs", "componentes", "ciclos", "bipartido", "articulacao"]
LOG = os.path.join("bin", "desenhos_log.csv")


def desenhar(entrada, nome, origem):
    for algo in ALGORITMOS:
        extra = ["--source", origem] if algo in ("bfs", "dfs") else []
        cmd = [GRAFOS, "--input", entrada, "--algo", algo, *extra,
               "--dot", f"results/dot/{nome}_{algo}.dot", "--log", LOG]
        if subprocess.run(cmd, stdout=subprocess.DEVNULL).returncode != 0:
            sys.exit(1)


def main():
    ir_para_raiz()
    compilar()

    desenhar("tests/grafos/conferencia_g1.txt", "conferencia_g1", "0")
    desenhar("tests/grafos/conferencia_g2.txt", "conferencia_g2", "0")
    desenhar("data/samples/bfs_9487272_n100.txt", "amostra_n100", "9487272")
    if os.path.exists(LOG):
        os.remove(LOG)

    print(f"{len(glob.glob('results/dot/*.dot'))} arquivos .dot em results/dot/")
    with contextlib.redirect_stdout(io.StringIO()):   # a lista de PNGs não interessa aqui
        falhas = renderizar("results/dot")
    if falhas:
        sys.exit(1)
    print("PNGs renderizados ao lado de cada .dot")


if __name__ == "__main__":
    main()
