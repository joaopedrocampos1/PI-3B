#!/usr/bin/env python3
"""Renderiza em PNG todos os arquivos .dot de results/dot/ (ou da pasta dada),
ao lado de cada um: results/dot/x.dot -> results/dot/x.png.

Os .dot com mais de 30 vértices já declaram o layout sfdp (por forças);
os menores usam o layout em camadas, o padrão do Graphviz.

Uso, a partir de qualquer pasta:
  python3 scripts/render_dot.py [pasta] [dpi]

Requer o Graphviz (no Ubuntu: sudo apt install graphviz; no Windows:
https://graphviz.org/download/, marcando a opção de pôr o `dot` no PATH).
"""

import glob
import os
import shutil
import subprocess
import sys

from comum import ir_para_raiz


def renderizar(pasta="results/dot", dpi="50"):
    """Devolve o número de .dot que falharam."""
    if not shutil.which("dot"):
        sys.exit("erro: o Graphviz não está instalado (o comando `dot` não está no PATH)")

    arquivos = sorted(glob.glob(os.path.join(pasta, "*.dot")))
    if not arquivos:
        print(f"nenhum .dot em {pasta}")
        return 0

    falhas = 0
    for arquivo in arquivos:
        png = arquivo[: -len(".dot")] + ".png"
        if subprocess.run(["dot", "-Tpng", f"-Gdpi={dpi}", arquivo, "-o", png]).returncode == 0:
            print(f"  {png}")
        else:
            print(f"  falhou: {arquivo}", file=sys.stderr)
            falhas += 1
    return falhas


def main():
    ir_para_raiz()
    pasta = sys.argv[1] if len(sys.argv) > 1 else "results/dot"
    dpi = sys.argv[2] if len(sys.argv) > 2 else "50"
    sys.exit(1 if renderizar(pasta, dpi) else 0)


if __name__ == "__main__":
    main()
