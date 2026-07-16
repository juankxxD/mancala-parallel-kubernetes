#include "board.hpp"

#include <numeric>
#include <sstream>

namespace mancala {

Board Board::initial() {
    Board b;
    for (int i = 0; i < NUM_PITS; ++i) b.pits[i] = 0;
    for (int i = 0; i < PITS_PER_SIDE; ++i) {
        b.pits[i] = 4;
        b.pits[i + 7] = 4;
    }
    b.side_to_move = 0;
    return b;
}

std::vector<int> Board::legal_moves() const {
    std::vector<int> moves;
    moves.reserve(PITS_PER_SIDE);
    const int base = side_to_move == 0 ? 0 : 7;
    for (int i = 0; i < PITS_PER_SIDE; ++i) {
        if (pits[base + i] > 0) moves.push_back(base + i);
    }
    return moves;
}

bool Board::apply_move(int pit_abs) {
    const int side = side_to_move;
    const int my_kalaha = kalaha_of(side);
    const int opp_kalaha = kalaha_of(opponent(side));

    int seeds = pits[pit_abs];
    pits[pit_abs] = 0;
    int idx = pit_abs;
    while (seeds > 0) {
        idx = (idx + 1) % NUM_PITS;
        if (idx == opp_kalaha) continue;  // se salta el kalaha del rival
        pits[idx] += 1;
        seeds -= 1;
    }

    bool extra_turn = (idx == my_kalaha);

    // Captura: última semilla en hoyo propio vacío (que ahora vale 1) y el
    // hoyo opuesto del rival tiene semillas.
    if (!extra_turn) {
        const bool en_lado_propio = (side == 0)
            ? (idx >= 0 && idx <= 5)
            : (idx >= 7 && idx <= 12);
        if (en_lado_propio && pits[idx] == 1) {
            const int opp = opposite_pit(idx);
            if (pits[opp] > 0) {
                pits[my_kalaha] += pits[opp] + 1;
                pits[opp] = 0;
                pits[idx] = 0;
            }
        }
    }

    if (!extra_turn) {
        side_to_move = opponent(side);
    }

    // Si el lado que ahora tiene el turno no tiene jugadas legales, el
    // juego termina por collect (collect_remaining ajusta los kalahas).
    return extra_turn;
}

bool Board::terminal() const {
    return side_total(0) == 0 || side_total(1) == 0;
}

void Board::collect_remaining() {
    int s0 = 0, s1 = 0;
    for (int i = 0; i < PITS_PER_SIDE; ++i) {
        s0 += pits[i];
        pits[i] = 0;
        s1 += pits[i + 7];
        pits[i + 7] = 0;
    }
    pits[KALAHA_P0] += s0;
    pits[KALAHA_P1] += s1;
}

int Board::winner() const {
    if (pits[KALAHA_P0] > pits[KALAHA_P1]) return 1;
    if (pits[KALAHA_P0] < pits[KALAHA_P1]) return -1;
    return 0;
}

int Board::side_total(int side) const {
    const int base = side == 0 ? 0 : 7;
    int s = 0;
    for (int i = 0; i < PITS_PER_SIDE; ++i) s += pits[base + i];
    return s;
}

std::string Board::debug_str() const {
    std::ostringstream os;
    os << "    ";
    for (int i = 12; i >= 7; --i) os << pits[i] << ' ';
    os << "\n[" << pits[KALAHA_P1] << "]                   [" << pits[KALAHA_P0] << "]\n";
    os << "    ";
    for (int i = 0; i <= 5; ++i) os << pits[i] << ' ';
    os << "\nturn=" << side_to_move;
    return os.str();
}

}  // namespace mancala
