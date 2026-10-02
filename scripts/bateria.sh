#!/usr/bin/env bash
# Bateria de testes de estresse (#27), seguindo o protocolo da Metodologia do
# artigo (#26):
#
#   13 instâncias (as 12 amostras de data/samples e o grafo completo)
#   x 2 representações (lista e matriz)
#   x 6 algoritmos (bfs, dfs, componentes, ciclos, bipartido, articulacao),
#   cada configuração executada 11 vezes seguidas; a 1ª é o aquecimento.
#
# BFS e DFS partem da semente da própria amostra; no grafo completo, de cada
# uma das três sementes. Cada execução acrescenta uma linha a results/log.csv
# (componentes acrescenta duas: fortes e fracos). Uma configuração que falha,
# como a matriz sem memória, é registrada em results/inviaveis.csv e a bateria
# segue: o protocolo trata esse limite como resultado.
#
# Uso, a partir de qualquer pasta:
#   ./scripts/bateria.sh
#
# Para um teste rápido, sem seguir o protocolo inteiro:
#   REPETICOES=2 LOG=bin/log.csv INVIAVEIS=bin/inviaveis.csv \
#   INSTANCIAS="data/samples/bfs_9487272_n100.txt" ./scripts/bateria.sh

set -u
cd "$(dirname "$0")/.." || exit 1

COMPLETO=data/raw/twitter_combined.txt
SEMENTES="307642294 144319796 9487272"   # sorteadas com --semente-rng 2026
ALGORITMOS="bfs dfs componentes ciclos bipartido articulacao"
ESTRUTURAS="lista matriz"

REPETICOES=${REPETICOES:-11}
LOG=${LOG:-results/log.csv}
INVIAVEIS=${INVIAVEIS:-results/inviaveis.csv}
MAQUINA=${MAQUINA:-results/maquina.txt}
INSTANCIAS=${INSTANCIAS:-"$(ls data/samples/bfs_*.txt) $COMPLETO"}

if [ -s "$LOG" ]; then
    echo "erro: $LOG já existe. A bateria grava um log limpo; mova ou apague o atual antes." >&2
    exit 1
fi
for f in $INSTANCIAS; do
    if [ ! -f "$f" ]; then
        echo "erro: $f não existe (o grafo completo vem de scripts/baixar_dataset.sh)" >&2
        exit 1
    fi
done
make -s bin/grafos || exit 1

# Especificação da máquina, exigida pelo protocolo.
{
    echo "data: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "cpu: $(lscpu | sed -n 's/^Model name:[[:space:]]*//p')"
    echo "nucleos: $(nproc)"
    echo "memoria: $(free -h | awk '/^Mem:/ {print $2}')"
    echo "so: $(. /etc/os-release && echo "$PRETTY_NAME"), kernel $(uname -r)"
    echo "compilador: $(gcc --version | head -1)"
    echo "flags: $(sed -n 's/^CFLAGS[[:space:]]*=[[:space:]]*//p' Makefile)"
    echo "repeticoes: $REPETICOES (a 1ª é o aquecimento)"
} > "$MAQUINA"

[ -s "$INVIAVEIS" ] || echo "dataset,estrutura,algoritmo,origem,codigo" > "$INVIAVEIS"

# A semente de uma amostra está no nome: bfs_<semente>_n<N>.txt
origens_de() {
    case "$1" in
        *bfs_*_n*) basename "$1" | sed 's/^bfs_\([0-9]*\)_n.*/\1/' ;;
        *) echo "$SEMENTES" ;;
    esac
}

# Lista as configurações, uma por linha: instância estrutura algoritmo origem
configuracoes() {
    for inst in $INSTANCIAS; do
        for est in $ESTRUTURAS; do
            for algo in $ALGORITMOS; do
                case "$algo" in
                    bfs | dfs) for o in $(origens_de "$inst"); do echo "$inst $est $algo $o"; done ;;
                    *) echo "$inst $est $algo -" ;;
                esac
            done
        done
    done
}

total=$(configuracoes | wc -l)
feitas=0
inicio=$(date +%s)
echo "bateria: $total configurações x $REPETICOES execuções, log em $LOG"

while read -r inst est algo origem; do
    feitas=$((feitas + 1))
    args=(--input "$inst" --struct "$est" --algo "$algo" --log "$LOG")
    [ "$origem" != "-" ] && args+=(--source "$origem")

    for exec in $(seq 1 "$REPETICOES"); do
        ./bin/grafos "${args[@]}" --execucao "$exec" > /dev/null 2>> bin/bateria_erros.txt
        codigo=$?
        if [ "$codigo" -ne 0 ]; then
            echo "$inst,$est,$algo,$origem,$codigo" >> "$INVIAVEIS"
            echo "  inviável: $inst $est $algo (código $codigo)"
            break
        fi
    done

    decorrido=$(( $(date +%s) - inicio ))
    printf '[%3d/%d] %-42s %-6s %-12s %5ds\n' "$feitas" "$total" "$(basename "$inst")" "$est" "$algo" "$decorrido"
done < <(configuracoes)

echo "bateria concluída em $(( $(date +%s) - inicio )) s: $(($(wc -l < "$LOG") - 1)) linhas em $LOG"
