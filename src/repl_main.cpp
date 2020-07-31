#include <iostream>
#include "reader.hpp"
#include "printer.hpp"
#include "interpreter.hpp"

std::size_t mal::RefCounter::total_refs = 0;

typedef mal::MalValue repl_expr;
typedef std::string repl_src;

static const std::string repl_prompt = "> ";

// ! Assuming stdout is actually a TTY
static mal::TTYPrinter printer{std::cout};

inline repl_expr read(const repl_src& src, mal::StringInternPool* str_interner) {
    return mal::ReadForm(src, str_interner);
}

inline repl_expr eval(const repl_expr& expr, mal::Interpreter& interp) {
    return interp.EvaluateExpression(expr, interp.env_global);
}

inline void print(const repl_expr& expr) {
    printer << mal::print_begin << expr << mal::print_end;
}

inline void rep(const repl_src& src, mal::Interpreter& interp) {
    mal::MalValue v = eval(read(src, &interp.str_interner), interp);
    if (!mh::is_nil(v))
        print(v);
}

inline mal::MalValue re(const repl_src& src, mal::Interpreter& interp) {
    return eval(read(src, &interp.str_interner), interp);
}

int main(int argc, char** argv) {
    mal::Interpreter interp{printer};
    {
        // Parse arguments into *ARGV*
        mal::ListBuilder arg_lb;
        for (int arg_i = 0; arg_i < argc; ++arg_i) {
            char* arg_s = argv[arg_i];
            arg_lb.push(mh::string(arg_s));
        }
        interp.env_global->set("*ARGV*", mh::list(arg_lb.release()));
    }
    try {
        re("(def load-file (fn (fName) (eval (read-string (str \"(do \" (slurp fName) \")\")))))", interp);
        if (!mh::is_true(re("(load-file \"bootstrap.mal\")", interp)))
            return 0;
    } catch (const mal::mal_error& err) {
        printer << mal::print_begin << "Script Mal Error: " << err.msg << mal::print_end;
        return 1;
    }
    // The REPL
    std::cout << "Mal Repl v.0.9" << std::endl;
    for (;;) {
        std::string line;

        std::cout << repl_prompt << std::flush;
        std::getline(std::cin, line);
        if (std::cin.eof())
            break;
        if (line.empty())
            continue;
        try {
            rep(line, interp);
        } catch (const mal::mal_error& err) {
            printer << mal::print_begin << "Mal Error: " << err.msg << mal::print_end;
        }
    }
    return 0;
}
