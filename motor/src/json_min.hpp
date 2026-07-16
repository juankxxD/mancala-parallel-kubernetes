#pragma once

// Parser/serializador JSON minimalista, lo justo para el contrato del motor:
//   request : { "board":[...14...], "side":0|1, "depth":N, "threads":N? }
//   response: { "move":N, "evaluation":F, "elapsed_ms":N,
//               "stats":{ "nodes":N, "prunes":N }, "threads_used":N }
//
// No pretende ser un parser JSON completo: rechaza estructuras anidadas
// arbitrarias, comentarios, escapes Unicode, etc. Suficiente para el
// contrato cerrado del motor.

#include <cctype>
#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mancala::jsonm {

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Parser {
public:
    explicit Parser(const std::string& s) : s_(s) {}

    void skip_ws() {
        while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_;
    }

    void expect(char c) {
        skip_ws();
        if (i_ >= s_.size() || s_[i_] != c) {
            throw ParseError(std::string("expected '") + c + "' at " + std::to_string(i_));
        }
        ++i_;
    }

    bool consume(char c) {
        skip_ws();
        if (i_ < s_.size() && s_[i_] == c) { ++i_; return true; }
        return false;
    }

    std::string parse_string() {
        skip_ws();
        expect('"');
        std::string out;
        while (i_ < s_.size() && s_[i_] != '"') {
            if (s_[i_] == '\\' && i_ + 1 < s_.size()) {
                char e = s_[i_ + 1];
                switch (e) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    default: throw ParseError("unsupported escape");
                }
                i_ += 2;
            } else {
                out += s_[i_++];
            }
        }
        expect('"');
        return out;
    }

    double parse_number() {
        skip_ws();
        size_t start = i_;
        if (i_ < s_.size() && (s_[i_] == '-' || s_[i_] == '+')) ++i_;
        while (i_ < s_.size() && (std::isdigit(static_cast<unsigned char>(s_[i_])) ||
                                  s_[i_] == '.' || s_[i_] == 'e' || s_[i_] == 'E' ||
                                  s_[i_] == '-' || s_[i_] == '+')) ++i_;
        if (start == i_) throw ParseError("expected number");
        return std::stod(s_.substr(start, i_ - start));
    }

    std::vector<int> parse_int_array() {
        std::vector<int> out;
        expect('[');
        skip_ws();
        if (consume(']')) return out;
        while (true) {
            out.push_back(static_cast<int>(parse_number()));
            skip_ws();
            if (consume(',')) continue;
            expect(']');
            break;
        }
        return out;
    }

    size_t pos() const { return i_; }
    const std::string& src() const { return s_; }

private:
    const std::string& s_;
    size_t i_ = 0;
};

// Construcción de salida JSON sencilla. Soporta arrays y objetos anidados;
// `key()` y los emisores de valor dentro de un array añaden la coma cuando
// corresponde.
class Writer {
public:
    void begin_obj() {
        sep_if_in_array();
        os_ << '{'; first_.push_back(true); is_arr_.push_back(false);
    }
    void end_obj()   { os_ << '}'; first_.pop_back(); is_arr_.pop_back(); }
    void begin_arr() {
        sep_if_in_array();
        os_ << '['; first_.push_back(true); is_arr_.push_back(true);
    }
    void end_arr()   { os_ << ']'; first_.pop_back(); is_arr_.pop_back(); }

    void key(const std::string& k) {
        if (!first_.back()) os_ << ',';
        first_.back() = false;
        os_ << '"' << k << "\":";
    }

    void str(const std::string& v)     { sep_if_in_array(); os_ << '"' << v << '"'; }
    void integer(std::int64_t v)       { sep_if_in_array(); os_ << v; }
    void number(double v) {
        sep_if_in_array();
        std::ostringstream tmp; tmp << v;
        os_ << tmp.str();
    }
    void boolean(bool v) { sep_if_in_array(); os_ << (v ? "true" : "false"); }

    std::string str_value() const { return os_.str(); }

private:
    void sep_if_in_array() {
        if (!is_arr_.empty() && is_arr_.back()) {
            if (!first_.back()) os_ << ',';
            first_.back() = false;
        }
    }

    std::ostringstream os_;
    std::vector<bool> first_;
    std::vector<bool> is_arr_;
};

}  // namespace mancala::jsonm
