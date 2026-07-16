#pragma once

// Servidor HTTP minimalista sin dependencias externas.
// Suficiente para el contrato del motor:
//   POST /move    -> JSON {move, evaluation, ...}
//   GET  /healthz -> 200 "ok"
//   GET  /readyz  -> 200 "ready"
// No soporta keep-alive ni chunked encoding; suficiente para uso interno
// del clúster con clientes simples (requests, fetch).

#include <functional>
#include <string>

namespace mancala::http {

struct Request {
    std::string method;
    std::string path;
    std::string body;
};

struct Response {
    int status = 200;
    std::string content_type = "application/json; charset=utf-8";
    std::string body;
};

using Handler = std::function<Response(const Request&)>;

// Inicia el servidor y bloquea hasta SIGTERM/SIGINT.
// `worker_threads` define cuántos hilos atienden conexiones (no se confunde
// con OMP_NUM_THREADS, que se aplica dentro de cada cálculo).
void serve(int port, int worker_threads, Handler handler);

}  // namespace mancala::http
