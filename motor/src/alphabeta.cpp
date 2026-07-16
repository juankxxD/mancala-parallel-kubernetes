#include "alphabeta.hpp"

#include <algorithm>
#include <limits>
#include <omp.h>

namespace mancala {

namespace {

constexpr int INF = std::numeric_limits<int>::max() / 2;

// Convención: el AB se hace siempre desde el punto de vista de `root_side`.
// Si en el nodo actual mueve `root_side` -> maximizando. Si mueve el rival
// -> minimizando. Esto unifica el manejo del turno extra (no alterna).
int ab(Board node, int depth, int alpha, int beta, int root_side,
       double alpha_weight, std::int64_t& nodes, std::int64_t& prunes) {
    ++nodes;
    if (node.terminal()) {
        Board copy = node;
        copy.collect_remaining();
        return (copy.pits[kalaha_of(root_side)] - copy.pits[kalaha_of(opponent(root_side))]) * 1000;
    }
    if (depth == 0) {
        return evaluate(node, root_side, alpha_weight);
    }

    const bool maximizando = (node.side_to_move == root_side);
    const auto moves = node.legal_moves();
    if (moves.empty()) {
        Board copy = node;
        copy.collect_remaining();
        return (copy.pits[kalaha_of(root_side)] - copy.pits[kalaha_of(opponent(root_side))]) * 1000;
    }

    if (maximizando) {
        int value = -INF;
        for (int m : moves) {
            Board child = node;
            child.apply_move(m);
            int v = ab(child, depth - 1, alpha, beta, root_side, alpha_weight, nodes, prunes);
            if (v > value) value = v;
            if (value > alpha) alpha = value;
            if (alpha >= beta) { ++prunes; break; }
        }
        return value;
    } else {
        int value = INF;
        for (int m : moves) {
            Board child = node;
            child.apply_move(m);
            int v = ab(child, depth - 1, alpha, beta, root_side, alpha_weight, nodes, prunes);
            if (v < value) value = v;
            if (value < beta) beta = value;
            if (alpha >= beta) { ++prunes; break; }
        }
        return value;
    }
}

}  // namespace

int evaluate(const Board& b, int root_side, double alpha_weight) {
    const int my_k = b.pits[kalaha_of(root_side)];
    const int opp_k = b.pits[kalaha_of(opponent(root_side))];
    const int my_side = b.side_total(root_side);
    const int opp_side = b.side_total(opponent(root_side));
    return (my_k - opp_k) + static_cast<int>(alpha_weight * (my_side - opp_side));
}

AlphaBetaResult search_alphabeta(const Board& root, const AlphaBetaConfig& cfg) {
    AlphaBetaResult result;
    const int root_side = root.side_to_move;
    const auto moves = root.legal_moves();
    if (moves.empty()) {
        return result;
    }

    if (cfg.threads <= 1) {
        // Alfa-Beta secuencial verdadero: la cota α se COMPARTE entre los hijos
        // de la raíz. El mejor valor encontrado hasta ahora estrecha la ventana
        // (alpha, +INF) de los siguientes hijos, lo que produce más podas dentro
        // de sus subárboles. Esta es la línea base T(1) que sí poda en la raíz;
        // el root parallelism la pierde al dar ventana completa a cada hilo.
        int best_val = -INF;
        int best_move = moves.front();
        int alpha = -INF;
        for (int m : moves) {
            Board child = root;
            child.apply_move(m);
            std::int64_t n = 0, p = 0;
            int v = ab(child, cfg.depth - 1, alpha, INF, root_side, cfg.alpha_weight, n, p);
            result.nodes += n;
            result.prunes += p;
            if (v > best_val) { best_val = v; best_move = m; }
            if (best_val > alpha) alpha = best_val;
        }
        result.move = best_move;
        result.evaluation = best_val;
        return result;
    }

    // --- Root parallelism con OpenMP ---
    // Cada hilo evalúa un subárbol de un movimiento legal raíz. No se
    // comparten cotas α/β entre hilos (limitación conocida del esquema):
    // se pierden algunas podas comparado con la versión secuencial, pero
    // mantiene la corrección (mismo movimiento óptimo a igual profundidad).
    const int n = static_cast<int>(moves.size());
    std::vector<int> values(n, -INF);
    std::vector<std::int64_t> nodes_v(n, 0), prunes_v(n, 0);

    #pragma omp parallel for num_threads(cfg.threads) schedule(dynamic, 1)
    for (int i = 0; i < n; ++i) {
        Board child = root;
        child.apply_move(moves[i]);
        std::int64_t local_n = 0, local_p = 0;
        int v = ab(child, cfg.depth - 1, -INF, INF, root_side,
                   cfg.alpha_weight, local_n, local_p);
        values[i] = v;
        nodes_v[i] = local_n;
        prunes_v[i] = local_p;
    }

    int best_i = 0;
    int best_val = values[0];
    for (int i = 1; i < n; ++i) {
        if (values[i] > best_val) { best_val = values[i]; best_i = i; }
    }
    for (int i = 0; i < n; ++i) {
        result.nodes += nodes_v[i];
        result.prunes += prunes_v[i];
    }
    result.move = moves[best_i];
    result.evaluation = best_val;
    return result;
}

}  // namespace mancala
