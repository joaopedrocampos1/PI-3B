#!/usr/bin/env bash
# Renderiza em PNG todos os arquivos .dot de results/dot/ (ou da pasta dada),
# ao lado de cada um: results/dot/x.dot -> results/dot/x.png.
#
# Os .dot com mais de 30 vértices já declaram o layout sfdp (por forças);
# os menores usam o layout em camadas, o padrão do Graphviz.
#
# Uso, a partir de qualquer pasta:
#   ./scripts/render_dot.sh [pasta] [dpi]
#
# Requer o Graphviz (no Ubuntu: sudo apt install graphviz).

set -u
cd "$(dirname "$0")/.." || exit 1

PASTA=${1:-results/dot}
DPI=${2:-50}

if ! command -v dot > /dev/null; then
    echo "erro: o Graphviz não está instalado (sudo apt install graphviz)" >&2
    exit 1
fi

falhas=0
for arquivo in "$PASTA"/*.dot; do
    [ -e "$arquivo" ] || { echo "nenhum .dot em $PASTA"; exit 0; }
    if dot -Tpng -Gdpi="$DPI" "$arquivo" -o "${arquivo%.dot}.png"; then
        echo "  ${arquivo%.dot}.png"
    else
        echo "  falhou: $arquivo" >&2
        falhas=$((falhas + 1))
    fi
done
[ "$falhas" -eq 0 ]
