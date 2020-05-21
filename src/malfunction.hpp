#pragma once

#include "malvalue.hpp"
#include "interpreter.hpp"
#include <vector>

namespace mal {
    class Environment;
    
    class MalFunction {
    public:
        std::vector<MalString::string_t> params;
        MalString::string_t param_var;
        std::shared_ptr<Environment> env;
        MalValue body;
        enum FKind {
            KFunc = 0,
            KMacro = 1,
        } kind;

        MalFunction(std::vector<MalString::string_t>&& params, MalString::string_t param_var, std::shared_ptr<Environment> env, const MalValue& body, FKind kind = KFunc)
          : params{std::move(params)}, param_var{std::move(param_var)}, env{std::move(env)}, body{body}, kind{kind} {}

        static std::shared_ptr<MalFunction> Make(std::vector<MalString::string_t>&& params, MalString::string_t param_var, std::shared_ptr<Environment> env, const MalValue& body, FKind kind = KFunc) {
            return std::make_shared<MalFunction>(std::move(params), std::move(param_var), std::move(env), body, kind);
        }

        bool IsVariadic() const {
            return param_var != "";
        }
    };
}
