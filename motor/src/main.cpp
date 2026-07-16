// Punto de entrada del motor. Dos modos:
//   --serve [--port N] [--workers N]   Servidor HTTP (default).
//   (sin args)                          Muestra ayuda y termina.

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "api.hpp"
#include "http_server.hpp"

namespace {

void usage(const char* prog) {
    std::cerr <<
        "Motor Kalah(6,4) — C++/OpenMP\n"
        "Uso:\n"
        "  " << prog << " --serve [--port N] [--workers N]\n"
        "      Sirve HTTP en :N (default 9000) con N workers (default 8).\n"
        "      Endpoints: GET /healthz, GET /readyz, POST /move\n"
        "\n"
        "Para benchmarks usar el binario 'mancala_bench'.\n";
}

mancala::http::Response handle(const mancala::http::Request& req) {
    mancala::http::Response r;
    if (req.method == "GET" && req.path == "/healthz") {
        r.status = 200; r.content_type = "application/json";
        r.body = "{\"status\":\"ok\"}";
        return r;
    }
    if (req.method == "GET" && req.path == "/readyz") {
        r.status = 200; r.content_type = "application/json";
        r.body = "{\"status\":\"ready\"}";
        return r;
    }
    if (req.method == "POST" && req.path == "/move") {
        auto out = mancala::api::handle_move(req.body);
        r.status = out.status; r.body = out.body;
        return r;
    }
    r.status = 404;
    r.body = "{\"error\":\"not found\"}";
    return r;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 1; }

    int port = 9000;
    int workers = 8;
    bool serve = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--serve") serve = true;
        else if (a == "--port" && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (a == "--workers" && i + 1 < argc) workers = std::atoi(argv[++i]);
        else if (a == "-h" || a == "--help") { usage(argv[0]); return 0; }
        else { std::cerr << "[motor] argumento desconocido: " << a << "\n"; usage(argv[0]); return 2; }
    }
    if (!serve) { usage(argv[0]); return 1; }

    mancala::http::serve(port, workers, handle);
    return 0;
}
