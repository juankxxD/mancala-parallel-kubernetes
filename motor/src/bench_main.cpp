// Modo benchmark del motor. Ejecuta Minimax + Alfa-Beta sobre un conjunto de
// posiciones (formato: una posición por línea, 14 enteros separados por
// espacios, seguidos del lado 0/1) y reporta:
//   - tiempo total T(p)
//   - métricas específicas de Alfa-Beta (nodos explorados y podas)
// El speedup S(p)=T(1)/T(p) y la eficiencia E(p)=S(p)/p se calculan comparando
// varias corridas (lo hace motor/bench/run_benchmarks.sh).
//
// Uso:
//   mancala_bench --depth 12 --threads 8 --positions tests/suite.txt

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <omp.h>

#include "alphabeta.hpp"
#include "board.hpp"

namespace {

struct Args {
    int depth = 12;
    int threads = 1;
    std::string positions_file = "tests/suite.txt";
};

void usage(const char* prog) {
    std::cerr <<
        "Uso: " << prog << " [opciones]\n"
        "  --depth N           profundidad de Alfa-Beta (def 12)\n"
        "  --threads N         hilos OpenMP (def 1)\n"
        "  --positions FILE    archivo con posiciones (def tests/suite.txt)\n";
}

std::vector<mancala::Board> load_positions(const std::string& path) {
    std::vector<mancala::Board> out;
    std::ifstream in(path);
    if (!in) {
        std::cerr << "[bench] no se puede abrir " << path
                  << "; usando posición inicial.\n";
        out.push_back(mancala::Board::initial());
        return out;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        mancala::Board b;
        for (int i = 0; i < mancala::NUM_PITS; ++i) ss >> b.pits[i];
        ss >> b.side_to_move;
        if (ss.fail()) {
            std::cerr << "[bench] línea inválida ignorada: " << line << "\n";
            continue;
        }
        out.push_back(b);
    }
    if (out.empty()) out.push_back(mancala::Board::initial());
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&](int& dst) { if (i + 1 < argc) dst = std::atoi(argv[++i]); };
        auto next_s = [&](std::string& dst) { if (i + 1 < argc) dst = argv[++i]; };
        if (k == "--depth") next(a.depth);
        else if (k == "--threads") next(a.threads);
        else if (k == "--positions") next_s(a.positions_file);
        else if (k == "-h" || k == "--help") { usage(argv[0]); return 0; }
        else { std::cerr << "argumento desconocido: " << k << "\n"; usage(argv[0]); return 2; }
    }

    if (a.threads < 1) a.threads = 1;
    omp_set_num_threads(a.threads);

    auto positions = load_positions(a.positions_file);

    std::cout << "[bench] algo=alphabeta"
              << " threads=" << a.threads
              << " positions=" << positions.size() << "\n";

    const double t0 = omp_get_wtime();

    std::int64_t total_nodes = 0, total_prunes = 0;
    for (const auto& pos : positions) {
        mancala::AlphaBetaConfig cfg{a.depth, 0.1, a.threads};
        auto r = mancala::search_alphabeta(pos, cfg);
        total_nodes += r.nodes;
        total_prunes += r.prunes;
    }

    const double t1 = omp_get_wtime();
    const double elapsed = t1 - t0;

    std::cout << "[bench] T(" << a.threads << ") = " << elapsed << " s\n";
    std::cout << "[bench] depth=" << a.depth
              << " nodes_total=" << total_nodes
              << " prunes_total=" << total_prunes << "\n";

    // Línea final en CSV fácil de parsear: algo,threads,depth,T,nodes,prunes
    std::cout << "CSV,alphabeta,"
              << a.threads << ","
              << a.depth << ","
              << elapsed << ","
              << total_nodes << ","
              << total_prunes << "\n";

    return 0;
}
