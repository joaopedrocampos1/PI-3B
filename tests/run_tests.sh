#!/usr/bin/env bash
# Compila e roda todos os testes de tests/ (#13).
#
# Uso, de qualquer pasta:
#   ./tests/run_tests.sh
#
# Sai com 0 só se todos os testes compilarem e passarem.

set -u
cd "$(dirname "$0")/.." || exit 1

make -s $(ls tests/test_*.c | sed 's|tests/\(.*\)\.c|bin/\1|') || {
    echo "falha ao compilar os testes"
    exit 1
}

total=0
falharam=()
for t in bin/test_*; do
    [ -x "$t" ] || continue
    total=$((total + 1))
    echo "== $t"
    if ! "./$t"; then
        falharam+=("$t")
    fi
done

echo
if [ ${#falharam[@]} -eq 0 ]; then
    echo "todos os $total testes passaram"
    exit 0
fi
echo "${#falharam[@]} de $total teste(s) falharam: ${falharam[*]}"
exit 1
