#!/usr/bin/env bash
#
# Barrido de benchmarks del motor para la sección de instrumentación del informe.
#
# Recorre p in {1,2,4,8} hilos en dos profundidades de Alfa-Beta, mide el tiempo
# de pared y calcula el speedup S(p)=T(1)/T(p) y la eficiencia E(p)=S(p)/p.
# Imprime tablas en Markdown listas para pegar en docs/03-paralelizacion.md.
#
# Uso:
#   ./run_benchmarks.sh                 # usa la suite por defecto
#   POSITIONS=ruta/otra.txt ./run_benchmarks.sh
#
# Requiere OpenMP real (Linux con g++/libgomp es lo recomendado). En macOS con
# Apple clang hay que instalar libomp; si no, el binario corre en un solo hilo y
# el speedup saldrá plano.

set -euo pipefail

# Ubicarse en la carpeta motor/ (este script vive en motor/bench/).
cd "$(dirname "$0")/.."

POSITIONS="${POSITIONS:-tests/suite.txt}"
THREADS_LIST=(1 2 4 8)
AB_DEPTHS=(8 12)

# Compilar en modo Release.
echo "Compilando el motor (Release)..." >&2
cmake -S src -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build --parallel >/dev/null
BENCH=build/mancala_bench

# Extrae el tiempo (campo 5) de la línea 'CSV,...' que imprime el benchmark.
run_one() {
    local depth="$1" threads="$2"
    OMP_NUM_THREADS="$threads" "$BENCH" --depth "$depth" \
        --threads "$threads" --positions "$POSITIONS" \
        | awk -F, '/^CSV,/ {print $5}'
}

# Imprime una tabla Markdown T(p)/S(p)/E(p) para una profundidad dada.
emit_table() {
    local depth="$1" title="$2"
    echo
    echo "$title"
    echo
    echo "| p | T(p) [s] | S(p) | E(p) |"
    echo "|---|---|---|---|"
    local t1=""
    for p in "${THREADS_LIST[@]}"; do
        local tp; tp="$(run_one "$depth" "$p")"
        if [[ -z "$t1" ]]; then t1="$tp"; fi
        awk -v p="$p" -v tp="$tp" -v t1="$t1" \
            'BEGIN { s = t1/tp; e = s/p; printf "| %d | %.3f | %.2f | %.2f |\n", p, tp, s, e }'
    done
}

echo "# Resultados del barrido (pegar en docs/03-paralelizacion.md)"
for d in "${AB_DEPTHS[@]}"; do
    emit_table "$d" "Alfa-Beta, \`depth=$d\`:"
done
