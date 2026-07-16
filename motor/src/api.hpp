#pragma once

// Capa de adaptación entre JSON y los algoritmos.
// Reusada por el modo --serve (HTTP) y por los tests.

#include <string>

#include "board.hpp"

namespace mancala::api {

// Recibe el body JSON de POST /move y devuelve el JSON de respuesta.
// Errores se mapean a respuestas con campo "error" y código HTTP en `status`.
struct ApiResult {
    int status = 200;
    std::string body;
};

ApiResult handle_move(const std::string& json_body);

// Helper independiente: serializa un Board a JSON {"board":[...], "side":N}.
std::string board_to_json(const Board& b);

}  // namespace mancala::api
