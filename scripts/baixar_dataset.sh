#!/usr/bin/env bash
# Baixa o dataset ego-Twitter do SNAP para data/raw/ e confere a integridade.
# O arquivo não é versionado (44 MB descompactado); rode este script após clonar.
set -euo pipefail

URL="https://snap.stanford.edu/data/twitter_combined.txt.gz"
SHA256="d9f99b0e6a53b9204b8c215f41b3c10fb99a1e1e783858c012b06d0d3d4bd129"

DIR="$(cd "$(dirname "$0")/.." && pwd)/data/raw"
GZ="$DIR/twitter_combined.txt.gz"

mkdir -p "$DIR"

if [ ! -f "$GZ" ]; then
    echo "Baixando $URL"
    curl -fL --progress-bar -o "$GZ" "$URL"
fi

echo "$SHA256  $GZ" | sha256sum -c -

gunzip -kf "$GZ"
echo "Pronto: $DIR/twitter_combined.txt"
