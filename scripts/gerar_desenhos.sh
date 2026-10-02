#!/usr/bin/env bash
# Gera os desenhos versionados em results/dot/ (.dot e .png): os seis
# algoritmos sobre os grafos da conferência manual (G1 e G2, a partir do
# vértice 0) e sobre uma amostra de 100 vértices (a partir da sua semente).
#
# Os .dot saem do programa (--dot); os PNG, de scripts/render_dot.sh, que
# requer o Graphviz. As execuções vão para um log descartável, para não
# misturar com results/log.csv da bateria.
#
# Uso, a partir de qualquer pasta:
#   ./scripts/gerar_desenhos.sh

set -u
cd "$(dirname "$0")/.." || exit 1
make -s bin/grafos || exit 1

ALGORITMOS="bfs dfs componentes ciclos bipartido articulacao"
LOG=bin/desenhos_log.csv

desenhar() {   # entrada nome origem
    for algo in $ALGORITMOS; do
        extra=()
        case "$algo" in bfs | dfs) extra=(--source "$3") ;; esac
        ./bin/grafos --input "$1" --algo "$algo" "${extra[@]}" \
            --dot "results/dot/$2_$algo.dot" --log "$LOG" > /dev/null || exit 1
    done
}

desenhar tests/grafos/conferencia_g1.txt conferencia_g1 0
desenhar tests/grafos/conferencia_g2.txt conferencia_g2 0
desenhar data/samples/bfs_9487272_n100.txt amostra_n100 9487272
rm -f "$LOG"

echo "$(ls results/dot/*.dot | wc -l) arquivos .dot em results/dot/"
./scripts/render_dot.sh results/dot > /dev/null && echo "PNGs renderizados ao lado de cada .dot"
