#pragma once

#include <atomic>
#include <cstdint>

#include "board.hpp"

namespace mancala {

struct AlphaBetaResult {
    int move = -1;
    int evaluation = 0;
    std::int64_t nodes = 0;
    std::int64_t prunes = 0;
};

struct AlphaBetaConfig {
    int depth = 8;
    double alpha_weight = 0.1;   // peso α de la heurística h(estado)
    int threads = 1;             // 1 == secuencial; >1 activa root parallelism
};

// Heurística:
//   h(estado) = (kalaha_propio - kalaha_rival)
//             + alpha_weight * (semillas_propio - semillas_rival)
int evaluate(const Board& b, int root_side, double alpha_weight);

AlphaBetaResult search_alphabeta(const Board& root, const AlphaBetaConfig& cfg);

}  // namespace mancala
