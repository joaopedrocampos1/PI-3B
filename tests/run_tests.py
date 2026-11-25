#!/usr/bin/env python3
"""Compila e roda todos os testes de tests/ (#13). Ao contrário do
`make test`, não para no primeiro que falha: roda todos e lista os que
falharam no fim.

Uso, de qualquer pasta:
  python3 tests/run_tests.py

Sai com 0 só se todos os testes compilarem e passarem.
"""

import glob
import os
import shutil
import subprocess
import sys

RAIZ = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
EXE = ".exe" if os.name == "nt" else ""


def comando_make():
    if os.environ.get("MAKE"):
        return os.environ["MAKE"]
    for nome in ("make", "mingw32-make"):
        if shutil.which(nome):
            return nome
    sys.exit("erro: nem make nem mingw32-make estão no PATH")


def main():
    os.chdir(RAIZ)
    fontes = sorted(glob.glob(os.path.join("tests", "test_*.c")))
    alvos = ["bin/" + os.path.basename(f)[: -len(".c")] + EXE for f in fontes]
    if subprocess.run([comando_make(), "-s", *alvos]).returncode != 0:
        print("falha ao compilar os testes")
        sys.exit(1)

    falharam = []
    for alvo in alvos:
        print(f"== {alvo}", flush=True)
        if subprocess.run([os.path.join(*alvo.split("/"))]).returncode != 0:
            falharam.append(alvo)

    print()
    if not falharam:
        print(f"todos os {len(alvos)} testes passaram")
        sys.exit(0)
    print(f"{len(falharam)} de {len(alvos)} teste(s) falharam: {' '.join(falharam)}")
    sys.exit(1)


if __name__ == "__main__":
    main()
