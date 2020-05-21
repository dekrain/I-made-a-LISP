#pragma once

#include <iostream>

#include "malvalue.hpp"

namespace mal {
    struct print_begin_t { bool print_raw; };
    struct print_end_t {};

    constexpr print_begin_t print_begin{false};
    constexpr print_begin_t print_begin_raw{true};
    constexpr print_end_t print_end;

    class Printer {
    public:
        virtual Printer& operator<<(print_begin_t) = 0;
        virtual Printer& operator<<(print_end_t) = 0;
        virtual Printer& operator<<(const MalValue& value) = 0;
        virtual Printer& operator<<(const std::string& str) = 0;
    };

    class OstreamPrinter : public Printer {
        std::ostream& stream;
        bool is_raw;
    public:
        OstreamPrinter(std::ostream& stream) : stream{stream} {}

        Printer& operator<<(print_begin_t);
        Printer& operator<<(print_end_t);
        Printer& operator<<(const MalValue& value);
        Printer& operator<<(const std::string& str);
    };

    // Escape a string to represent special codes as escape sequences and with prefix/affix '"'
    std::string EscapeString(const std::string& str);
}