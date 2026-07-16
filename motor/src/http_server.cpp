#include "http_server.hpp"

#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  #define CLOSE_SOCKET closesocket
  #define SOCKET_ERROR_VAL INVALID_SOCKET
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <signal.h>
  #include <sys/socket.h>
  #include <unistd.h>
  using socket_t = int;
  #define CLOSE_SOCKET ::close
  #define SOCKET_ERROR_VAL (-1)
#endif

namespace mancala::http {

namespace {

std::string status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 422: return "Unprocessable Entity";
        case 500: return "Internal Server Error";
        case 503: return "Service Unavailable";
        default:  return "OK";
    }
}

std::string read_request(socket_t s, std::string& method, std::string& path) {
    std::string buf;
    buf.reserve(4096);
    char tmp[4096];
    // Lectura hasta tener cabeceras completas (\r\n\r\n).
    while (buf.find("\r\n\r\n") == std::string::npos) {
        int n = ::recv(s, tmp, sizeof(tmp), 0);
        if (n <= 0) return {};
        buf.append(tmp, static_cast<size_t>(n));
        if (buf.size() > 64 * 1024) return {};  // sanity cap
    }

    // Parsear request line.
    size_t sp1 = buf.find(' ');
    size_t sp2 = buf.find(' ', sp1 + 1);
    size_t eol = buf.find("\r\n", sp2 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos || eol == std::string::npos) return {};
    method = buf.substr(0, sp1);
    path = buf.substr(sp1 + 1, sp2 - sp1 - 1);

    // Content-Length.
    size_t cl = 0;
    {
        std::string lower = buf;
        for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        size_t k = lower.find("content-length:");
        if (k != std::string::npos) {
            size_t v_start = k + std::string("content-length:").size();
            while (v_start < lower.size() && lower[v_start] == ' ') ++v_start;
            cl = static_cast<size_t>(std::stoul(lower.substr(v_start)));
        }
    }

    size_t body_start = buf.find("\r\n\r\n") + 4;
    std::string body = buf.substr(body_start);
    while (body.size() < cl) {
        int n = ::recv(s, tmp, sizeof(tmp), 0);
        if (n <= 0) break;
        body.append(tmp, static_cast<size_t>(n));
    }
    if (body.size() > cl) body.resize(cl);
    return body;
}

void send_response(socket_t s, const Response& r) {
    std::ostringstream os;
    os << "HTTP/1.1 " << r.status << ' ' << status_text(r.status) << "\r\n";
    os << "Content-Type: " << r.content_type << "\r\n";
    os << "Content-Length: " << r.body.size() << "\r\n";
    os << "Connection: close\r\n\r\n";
    os << r.body;
    const std::string out = os.str();
    size_t sent = 0;
    while (sent < out.size()) {
        int n = ::send(s, out.data() + sent, static_cast<int>(out.size() - sent), 0);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
    }
}

void handle_connection(socket_t client, Handler handler) {
    Request req;
    req.body = read_request(client, req.method, req.path);
    Response resp;
    try {
        resp = handler(req);
    } catch (const std::exception& e) {
        resp.status = 500;
        resp.body = std::string("{\"error\":\"") + e.what() + "\"}";
    }
    send_response(client, resp);
    CLOSE_SOCKET(client);
}

}  // namespace

void serve(int port, int worker_threads, Handler handler) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[motor] WSAStartup failed\n"; return;
    }
#else
    ::signal(SIGPIPE, SIG_IGN);
#endif

    socket_t listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener == SOCKET_ERROR_VAL) {
        std::cerr << "[motor] socket() failed\n"; return;
    }
    int yes = 1;
    ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "[motor] bind(" << port << ") failed\n"; return;
    }
    if (::listen(listener, 64) != 0) {
        std::cerr << "[motor] listen() failed\n"; return;
    }
    std::cerr << "[motor] HTTP listening on :" << port
              << " (workers=" << worker_threads << ")\n";

    std::atomic<int> active_workers{0};
    while (true) {
        sockaddr_in caddr{};
        socklen_t clen = sizeof(caddr);
        socket_t client = ::accept(listener,
                                   reinterpret_cast<sockaddr*>(&caddr), &clen);
        if (client == SOCKET_ERROR_VAL) continue;

        // Limita el paralelismo de aceptación; cada handler puede a su vez
        // usar OpenMP internamente para el cálculo.
        while (active_workers.load() >= worker_threads) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        active_workers++;
        std::thread([client, handler, &active_workers]() {
            handle_connection(client, handler);
            active_workers--;
        }).detach();
    }
}

}  // namespace mancala::http
