#!/usr/bin/env python3
"""Baixa o dataset ego-Twitter do SNAP para data/raw/ e confere a integridade.

O arquivo não é versionado (44 MB descompactado); rode este script após clonar.

Uso, a partir de qualquer pasta:
  python3 scripts/baixar_dataset.py
"""

import gzip
import hashlib
import os
import shutil
import sys
import urllib.request

from comum import RAIZ

URL = "https://snap.stanford.edu/data/twitter_combined.txt.gz"
SHA256 = "d9f99b0e6a53b9204b8c215f41b3c10fb99a1e1e783858c012b06d0d3d4bd129"

PASTA = os.path.join(RAIZ, "data", "raw")
GZ = os.path.join(PASTA, "twitter_combined.txt.gz")
TXT = os.path.join(PASTA, "twitter_combined.txt")


def baixar():
    print(f"Baixando {URL}")
    parcial = GZ + ".parcial"
    with urllib.request.urlopen(URL) as resposta, open(parcial, "wb") as saida:
        total = int(resposta.headers.get("Content-Length") or 0)
        lidos = 0
        while True:
            bloco = resposta.read(1 << 16)
            if not bloco:
                break
            saida.write(bloco)
            lidos += len(bloco)
            if total:
                print(f"\r  {lidos * 100 // total:3d}% de {total / 1e6:.1f} MB", end="", flush=True)
    print()
    os.replace(parcial, GZ)   # só vira .gz quando o download termina inteiro


def sha256(caminho):
    h = hashlib.sha256()
    with open(caminho, "rb") as f:
        for bloco in iter(lambda: f.read(1 << 20), b""):
            h.update(bloco)
    return h.hexdigest()


def main():
    os.makedirs(PASTA, exist_ok=True)
    if not os.path.isfile(GZ):
        try:
            baixar()
        except OSError as erro:   # inclui urllib.error.URLError
            if os.path.exists(GZ + ".parcial"):
                os.remove(GZ + ".parcial")
            sys.exit(f"erro ao baixar {URL}: {erro}")

    if sha256(GZ) != SHA256:
        sys.exit(f"{GZ}: FALHOU (SHA-256 não confere; apague o arquivo e rode de novo)")
    print(f"{GZ}: OK")

    # descompacta mantendo o .gz, como o gunzip -kf
    with gzip.open(GZ, "rb") as entrada, open(TXT, "wb") as saida:
        shutil.copyfileobj(entrada, saida)
    print(f"Pronto: {TXT}")


if __name__ == "__main__":
    main()
