#pragma once

#include <memory>
#include <string>

#include <unordered_map>
#include <vector>

#include "malvalue.hpp"
#include "printer.hpp"
#include "invoke.hpp"

namespace mal {
    class Printer;    

    class Interpreter {
        void InitEnv();

        MalValue EvalAst(const MalValue& expr, const EnvironFrame& env);
        bool Apply(MalAtom& curr, EnvironFrame& env, const MalValue& func, std::shared_ptr<MalList> args);
        MalValue QuasiQuote(const MalValue& expr);

        // ! WARNING: Platform specific
        static constexpr std::size_t MAX_RECURSION_DEPTH = 500;
        std::size_t recursion_depth = 0;
    public:
        EnvironFrame env_global;
        Printer& printer;

        Interpreter(Printer& printer) : printer{printer} {
            InitEnv();
        }

        MalValue EvalFunction(const MalFunction& func, MalArgs&& args);
        MalValue EvaluateExpression(const MalValue& expr, EnvironFrame env);
        inline MalValue InvokeFunction(const MalValue& func, MalArgs&& args) { // func must be invokable
            if (func.tag == Builtin_T)
                return func.blt(*this, std::move(args));
            // Function_T
            return EvalFunction(*func.fun, std::move(args));
        }
    };

    /*struct interpreter {
        std::unique_ptr<mal_error> i_error;
    };

    thread_local interpreter _interp;

    inline interpreter& get_interpreter() {
        return _interp;
    }

    inline void set_error(const std::string& msg) {
        get_interpreter().i_error = std::make_unique<mal_error>(msg);
    }*/
}
