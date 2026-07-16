#pragma once

// Tablero de Kalah(6,4) y reglas de movimiento.
//
// Orden canónico del arreglo board[14]:
//   índices 0..5   : hoyos del jugador 0 (lado "propio" en convención inicial)
//   índice 6       : kalaha del jugador 0
//   índices 7..12  : hoyos del jugador 1
//   índice 13      : kalaha del jugador 1
//
// Recorrido antihorario: el jugador 0 siembra 0 -> 1 -> ... -> 5 -> kalaha 6 ->
// 7 -> ... -> 12 -> (salta el kalaha 13 del rival) -> 0 -> ...
// Para el jugador 1, simétricamente: 7 -> 8 -> ... -> 12 -> kalaha 13 ->
// 0 -> ... -> 5 -> (salta el kalaha 6) -> 7 -> ...

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mancala {

using Seeds = int;
constexpr int NUM_PITS = 14;
constexpr int PITS_PER_SIDE = 6;
constexpr int KALAHA_P0 = 6;
constexpr int KALAHA_P1 = 13;

struct Board {
    std::array<Seeds, NUM_PITS> pits{};
    int side_to_move = 0;  // 0 o 1

    static Board initial();

    // Hoyos legales (relativos al lado): índices absolutos donde
    // side_to_move tiene semillas en alguno de sus 6 hoyos.
    std::vector<int> legal_moves() const;

    // Aplica el movimiento `pit_abs` y devuelve si el jugador conserva el
    // turno (true) o si pasa al rival (false). El estado del tablero se
    // muta in-place.
    bool apply_move(int pit_abs);

    bool terminal() const;

    // Al terminar, recoge las semillas del lado no vacío en su kalaha.
    // Llamar solo cuando terminal() == true.
    void collect_remaining();

    // 1 si gana p0, -1 si gana p1, 0 si empate. Llama collect_remaining()
    // antes si el juego está terminado y aún no se han recogido las semillas.
    int winner() const;

    // Acceso a la suma de semillas por lado (sin contar kalahas).
    int side_total(int side) const;

    std::string debug_str() const;
};

inline int kalaha_of(int side) {
    return side == 0 ? KALAHA_P0 : KALAHA_P1;
}

inline int opponent(int side) {
    return 1 - side;
}

// Hoyo opuesto en el tablero (para la regla de captura).
inline int opposite_pit(int pit) {
    return 12 - pit;
}

}  // namespace mancala
