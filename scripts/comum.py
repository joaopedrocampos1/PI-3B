"""Funções comuns aos scripts do PI-3B (#58).

Só biblioteca padrão, para os scripts rodarem igual no Linux, no macOS e no
Windows (cmd, PowerShell ou Git Bash), sem bash nem comandos do sistema.
"""

import os
import shutil
import subprocess
import sys

RAIZ = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
WINDOWS = os.name == "nt"

# O executável: bin/grafos no Linux e no macOS, bin\grafos.exe no Windows.
GRAFOS = os.path.join("bin", "grafos.exe" if WINDOWS else "grafos")


def ir_para_raiz():
    """Os caminhos dos scripts são relativos à raiz do repositório, como
    era o `cd "$(dirname "$0")/.."` dos .sh."""
    os.chdir(RAIZ)


def comando_make():
    """`make` no Linux e no macOS; no Windows com MinGW, `mingw32-make`.
    A variável de ambiente MAKE, se definida, tem prioridade."""
    if os.environ.get("MAKE"):
        return os.environ["MAKE"]
    for nome in ("make", "mingw32-make"):
        if shutil.which(nome):
            return nome
    sys.exit("erro: nem make nem mingw32-make estão no PATH")


def compilar():
    """Compila bin/grafos (ou bin/grafos.exe), como o `make -s bin/grafos`."""
    alvo = GRAFOS.replace(os.sep, "/")
    if subprocess.run([comando_make(), "-s", alvo]).returncode != 0:
        sys.exit(1)
