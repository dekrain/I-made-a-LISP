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
    protected:
        std::ostream& stream;
        bool is_raw;
    public:
        OstreamPrinter(std::ostream& stream) : stream{stream} {}

        virtual Printer& operator<<(print_begin_t);
        virtual Printer& operator<<(print_end_t);
        virtual Printer& operator<<(const MalValue& value);
        virtual Printer& operator<<(const std::string& str);
    };

    struct TTYColors {
        using str = const char*;
        static constexpr str reset = "\e[0m";
        static constexpr str boolean = "\e[96m";
        static constexpr str nil = "\e[90m";
        static constexpr str number = "\e[93m";
        static constexpr str keyword = "\e[95m";
        static constexpr str string = "\e[92m";
    };

    class TTYPrinter : public OstreamPrinter {
    protected:
        using _Base = OstreamPrinter;
    public:
        using OstreamPrinter::OstreamPrinter;
        using OstreamPrinter::operator<<;

        virtual Printer& operator<<(const MalValue& value) override;
    };

    // Escape a string to represent special codes as escape sequences and with prefix/affix '"'
    std::string EscapeString(const std::string& str);
}