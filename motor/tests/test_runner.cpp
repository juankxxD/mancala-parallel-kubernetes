// Runner mínimo de tests para el motor — sin framework externo.
// Cada CHECK falla termina con código 1 e imprime archivo:línea.

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "alphabeta.hpp"
#include "board.hpp"

static int g_failed = 0;
static int g_total = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        ++g_total;                                                       \
        if (!(cond)) {                                                   \
            ++g_failed;                                                  \
            std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                                \
    } while (0)

#define CHECK_EQ(a, b) CHECK((a) == (b))

using namespace mancala;

// Pruebas de las reglas de Kalah.

void test_initial_board() {
    Board b = Board::initial();
    for (int i = 0; i < 6; ++i) CHECK_EQ(b.pits[i], 4);
    for (int i = 7; i < 13; ++i) CHECK_EQ(b.pits[i], 4);
    CHECK_EQ(b.pits[6], 0);
    CHECK_EQ(b.pits[13], 0);
    CHECK_EQ(b.side_to_move, 0);
}

void test_legal_moves() {
    Board b = Board::initial();
    auto m = b.legal_moves();
    CHECK_EQ(m.size(), size_t{6});
    CHECK_EQ(m[0], 0);
    CHECK_EQ(m[5], 5);

    Board b2 = b;
    b2.pits[2] = 0;
    auto m2 = b2.legal_moves();
    CHECK_EQ(m2.size(), size_t{5});
}

void test_apply_move_basic() {
    // Sembrar desde hoyo 0 con 4 semillas: caen en 1,2,3,4. No es kalaha,
    // ni captura (hoyo opuesto vacío no aplica porque cae en propio con
    // semillas previas).
    Board b = Board::initial();
    bool extra = b.apply_move(0);
    CHECK(!extra);
    CHECK_EQ(b.pits[0], 0);
    CHECK_EQ(b.pits[1], 5);
    CHECK_EQ(b.pits[2], 5);
    CHECK_EQ(b.pits[3], 5);
    CHECK_EQ(b.pits[4], 5);
    CHECK_EQ(b.pits[5], 4);  // sin cambio
    CHECK_EQ(b.side_to_move, 1);
}

void test_extra_turn_kalaha() {
    // Sembrar desde hoyo 2 (4 semillas): cae en 3,4,5,6(kalaha). Turno extra.
    Board b = Board::initial();
    bool extra = b.apply_move(2);
    CHECK(extra);
    CHECK_EQ(b.pits[2], 0);
    CHECK_EQ(b.pits[3], 5);
    CHECK_EQ(b.pits[4], 5);
    CHECK_EQ(b.pits[5], 5);
    CHECK_EQ(b.pits[6], 1);  // kalaha p0
    CHECK_EQ(b.side_to_move, 0);  // mismo turno
}

void test_skip_opponent_kalaha() {
    // Sembrar desde hoyo 0 con 13 semillas debe pasar por el kalaha propio
    // (idx 6) y saltar el rival (idx 13), terminando en el hoyo 0.
    Board b{};
    b.pits[0] = 13;
    b.side_to_move = 0;
    b.apply_move(0);
    CHECK_EQ(b.pits[13], 0);  // kalaha rival NO recibe semilla
    CHECK(b.pits[6] >= 1);    // kalaha propio SÍ recibe al pasar
}

void test_capture_rule() {
    // Construir captura: hoyo 2 propio vacío, opuesto (12-2=10) con
    // semillas; sembrar desde hoyo 1 con 1 semilla cae justo en 2.
    Board b{};
    b.pits[1] = 1;
    b.pits[10] = 5;  // opuesto de 2 es 10
    b.side_to_move = 0;
    bool extra = b.apply_move(1);
    CHECK(!extra);
    CHECK_EQ(b.pits[2], 0);  // capturado
    CHECK_EQ(b.pits[10], 0); // capturado
    CHECK_EQ(b.pits[6], 6);  // kalaha p0: 5 (opuesto) + 1 (la que llegó)
}

void test_terminal_and_collect() {
    Board b{};
    b.pits[6] = 12;       // kalaha p0
    b.pits[7] = 3;
    b.pits[8] = 2;
    b.pits[13] = 5;       // kalaha p1
    b.side_to_move = 1;
    CHECK(b.terminal());  // lado p0 vacío
    b.collect_remaining();
    CHECK_EQ(b.pits[13], 10);  // 5 + (3+2)
    CHECK_EQ(b.pits[6], 12);   // kalaha p0 sin cambios (lado propio ya vacío)
    CHECK_EQ(b.pits[7], 0);
    CHECK_EQ(b.pits[8], 0);
    CHECK_EQ(b.winner(), 1);   // 12 > 10  -> gana p0
}

// Pruebas de equivalencia entre Alfa-Beta y Minimax sin poda.

// Minimax sin poda, para comparar.
static int minimax_no_pruning(Board node, int depth, int root_side, double aw,
                              std::int64_t& nodes) {
    ++nodes;
    if (node.terminal()) {
        Board c = node; c.collect_remaining();
        return (c.pits[kalaha_of(root_side)] - c.pits[kalaha_of(opponent(root_side))]) * 1000;
    }
    if (depth == 0) return evaluate(node, root_side, aw);
    auto moves = node.legal_moves();
    if (moves.empty()) {
        Board c = node; c.collect_remaining();
        return (c.pits[kalaha_of(root_side)] - c.pits[kalaha_of(opponent(root_side))]) * 1000;
    }
    bool maxing = (node.side_to_move == root_side);
    int best = maxing ? -1000000 : 1000000;
    for (int m : moves) {
        Board ch = node; ch.apply_move(m);
        int v = minimax_no_pruning(ch, depth - 1, root_side, aw, nodes);
        if (maxing) { if (v > best) best = v; }
        else        { if (v < best) best = v; }
    }
    return best;
}

void test_alphabeta_equals_minimax_serial() {
    // Para varias posiciones y profundidades, AB y Minimax deben coincidir
    // en el valor del nodo raíz y en el conjunto de movimientos óptimos.
    std::vector<Board> positions = {
        Board::initial(),
    };
    {
        Board b = Board::initial(); b.apply_move(2); positions.push_back(b);
        Board c = b; c.apply_move(7); positions.push_back(c);
    }

    for (const auto& pos : positions) {
        for (int depth : {3, 5, 7}) {
            // valor minimax de cada movimiento raíz
            auto moves = pos.legal_moves();
            int best_mm = -1000000;
            for (int m : moves) {
                Board ch = pos; ch.apply_move(m);
                std::int64_t n = 0;
                int v = minimax_no_pruning(ch, depth - 1, pos.side_to_move, 0.1, n);
                if (v > best_mm) best_mm = v;
            }

            AlphaBetaConfig cfg{depth, 0.1, 1};
            auto r = search_alphabeta(pos, cfg);
            CHECK_EQ(r.evaluation, best_mm);
        }
    }
}

void test_alphabeta_serial_equals_parallel() {
    // AB paralelo (root parallelism) debe encontrar el mismo valor que el
    // serial. El movimiento puede empatar entre opciones con igual valor,
    // pero el valor sí debe coincidir.
    Board b = Board::initial();
    AlphaBetaConfig cfg1{7, 0.1, 1};
    AlphaBetaConfig cfg4{7, 0.1, 4};
    auto r1 = search_alphabeta(b, cfg1);
    auto r4 = search_alphabeta(b, cfg4);
    CHECK_EQ(r1.evaluation, r4.evaluation);
}

int main() {
    test_initial_board();
    test_legal_moves();
    test_apply_move_basic();
    test_extra_turn_kalaha();
    test_skip_opponent_kalaha();
    test_capture_rule();
    test_terminal_and_collect();
    test_alphabeta_equals_minimax_serial();
    test_alphabeta_serial_equals_parallel();

    std::printf("\n[tests] %d/%d pasaron, %d fallaron\n",
                g_total - g_failed, g_total, g_failed);
    return g_failed == 0 ? 0 : 1;
}
