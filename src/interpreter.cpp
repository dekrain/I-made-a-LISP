#include "interpreter.hpp"

namespace mal {
    MalValue CreateFunction(const std::shared_ptr<MalList>& args, const EnvironFrame& env, MalFunction::FKind kind) {
        if (args->GetSize() != 2)
            throw mal_error{"Function takes 2 arguments"};
        if (!mh::is_sequence(args->At(0)))
            throw mal_error{"Function takes a list/vector as first argument"};
        ListIterator it = args->At(0).li;
        std::vector<MalString::string_t> params;
        MalString::string_t param_var = "";
        while (it) {
            MalValue v = *it;
            ++it;
            if (v.tag != Symbol_T)
                throw mal_error{"Function only accepts symbol parameter names"};
            if (v.st->Get() == "&") {
                if (!it)
                    throw mal_error{"Expected variadic parameter name"};
                if ((*it).tag != Symbol_T)
                    throw mal_error{"Function only accepts symbol parameter names"};
                param_var = (*it).st->Get();
                break;
            }
            params.push_back(v.st->Get());
        }
        return MalFunction::Make(std::move(params), std::move(param_var), env, args->At(1), kind);
    }

    inline EnvironFrame PrepareFunctionCall(const MalFunction& func, MalArgs&& args) {
        if (func.IsVariadic() ? args.size() < func.params.size() : args.size() != func.params.size())
            throw mal_error{"Arguments count doesn't match function's parameter count"};
        EnvironFrame env = std::make_unique<Environment>(func.env);
        std::size_t i;
        for (i = 0; i < func.params.size(); ++i) {
            env->set(func.params[i], std::move(args)[i]);
        }
        if (func.IsVariadic()) {
            ListBuilder lb;
            for (; i < args.size(); ++i) {
                lb.push(std::move(args)[i]);
            }
            env->set(func.param_var, mh::list(lb.release()));
        }
        return env;
    }

    inline void RequireMeta(const MalValue& val) {
        if (mh::is_map(val.Meta()))
            return;
        // Override !! meta of the code value with an empty hash map
        const_cast<MalValue&>(val).SetMeta(mh::hash_map(MalMap::Make()));
    }

    inline void SetMeta(const MalValue& target, const std::string& name, const MalValue& val) {
        RequireMeta(target);
        auto map = target.Meta().Map();
        map->Set(mh::keyword(name), val);
    }

    inline MalValue GetMeta(const MalValue& target, const std::string& name) {
        RequireMeta(target);
        auto map = target.Meta().Map();
        auto v = map->Lookup(mh::keyword(name));
        return v == map->data.end() ? mh::nil : v->second.get();
    }

    MalValue Interpreter::EvalAst(const MalValue& expr, const EnvironFrame& env) {
        switch (expr.tag) {
            case Symbol_T:
                return env->lookup(expr.st->Get());
            case List_T:
            case Vector_T: {
                auto l = mh::map(*expr.li, [&env, this](const auto& v){return this->EvaluateExpression(v, env); });
                return expr.tag == List_T ? mh::list(l) : mh::vector(l);
            }
            default:
                /*if (mh::is_num(expr) || mh::is_nil(expr) || mh::is_true(expr) || mh::is_false(expr)) {
                    // Mark a constant
                    SetMeta(expr, "constant_value", expr);
                }*/
                auto ret = expr;
                ret.SetMeta(nullptr);
                return ret;
        }
    }

    MalValue Interpreter::QuasiQuote(const MalValue& expr) {
        /* (defun (flist? x) (and (list? x) (not (empty? x))))
            (defun (quasiquote x)
                (if (flist? x)
                    (if (= (first x) 'unquote)
                        (nth x 1)
                        (if (and (flist? (first x)) (= (first (first x)) 'splice-unquote))
                            (list 'concat (nth (first x) 1) (quasiquote (rest x)))
                            (list 'cons (quasiquote (first x)) (quasiquote (rest x)))))
                    (list 'quote x)) */
        /* Experimental definition availible in bootstrap.mal: cor-quasiquote */
        if (!mh::is_flist(expr))
            return mh::list(mh::cons(mh::symbol("quote"), mh::cons(MalValue(expr), nullptr)));
        const auto& l = expr.li;
        if (mh::is_symbol(l->First()) && l->First().st->Get() == "unquote") {
            return l->At(1);
        }
        if (mh::is_flist(l->First())) {
            const auto& fir = l->First().li;
            if (mh::is_symbol(fir->First()) && fir->First().st->Get() == "splice-unquote")  {
                return mh::list(mh::cons(env_global->data["concat"].get(), mh::cons(MalValue(fir->At(1)), mh::cons(QuasiQuote(mh::list(l->Rest())), nullptr))));
            }
        }
        return mh::list(mh::cons(env_global->data["cons"].get(), mh::cons(QuasiQuote(l->First()), mh::cons(QuasiQuote(mh::list(l->Rest())), nullptr))));
    }

#   define RET_VALUE(v) do { curr = v; return true; } while(false)
// Equivalent to: return EvaluateExpression(expr, new_env)
#   define RET_TCO(expr, new_env) do { curr = expr; env = new_env; return false; } while(false)
#   define MV std::move
    bool Interpreter::Apply(MalAtom& curr, EnvironFrame& env, const MalValue& func, std::shared_ptr<MalList> args) {
        // Check special values
        if (func.tag == Symbol_T) {
            const MalString::string_t& sym = func.st->Get();
            if (sym == "def") {
                if (args->GetSize() != 2)
                    throw mal_error{"Def! takes 2 arguments"};
                MalValue key = args->At(0);
                if (key.tag != Symbol_T)
                    throw mal_error{"Def! only accepts symbol keys"};
                MalValue val = EvaluateExpression(args->At(1), env);
                env->set(key.st->Get(), val);
                RET_VALUE(MV(val));
            }
            else if (sym == "let*") {
                if (args->GetSize() != 2)
                    throw mal_error{"Let* takes 2 arguments"};
                if (args->At(0).tag != List_T)
                    throw mal_error{"Let* takes a list as first argument"};
                auto e = std::make_shared<Environment>(env);
                ListIterator it = args->At(0).li;
                while (it) {
                    MalValue k = *it;
                    if (k.tag != Symbol_T)
                        throw mal_error{"Let* only accepts symbol keys"};
                    ++it;
                    if (!it)
                        throw mal_error{"Odd number of arguments"};
                    MalValue v = *it;
                    ++it;
                    e->set(k.st->Get(), EvaluateExpression(v, e));
                }
                RET_TCO(args->At(1), MV(e));
            }
            else if (sym == "do") {
                ListIterator it = args;
                if (!it)
                    RET_VALUE(mh::nil);
                MalAtom val{*it};
                ++it;
                while (it) {
                    EvaluateExpression(val.get(), env);
                    val = *it;
                    ++it;
                }
                RET_TCO(val.get(), env);
            }
            else if (sym == "if") {
                if (args->GetSize() != 3 && args->GetSize() != 2)
                    throw mal_error{"If takes 2 or 3 arguments"};
                MalValue res = EvaluateExpression(args->At(0), env);
                if (res.tag != Nil_T && res.tag != False_T)
                    RET_TCO(args->At(1), env);
                else if (args->GetSize() == 3)
                    RET_TCO(args->At(2), env);
                else
                    RET_VALUE(mh::nil);
            }
            else if (sym == "fn") {
                RET_VALUE(CreateFunction(args, env, MalFunction::KFunc));
            }
            else if (sym == "macro") {
                RET_VALUE(CreateFunction(args, env, MalFunction::KMacro));
            }
            else if (sym == "quote") {
                if (args->GetSize() != 1)
                    throw mal_error{"Quote takes 1 argument"};
                RET_VALUE(args->First());
            }
            else if (sym == "quasiquote") {
                if (args->GetSize() != 1)
                    throw mal_error{"QuasiQuote takes 1 argument"};
                RET_TCO(QuasiQuote(args->First()), env);
            }
            else if (sym == "macroexpand") {
                // ! WARNING: Can cause side effects (always evaluates the callee expression)
                // ! EXTRA WARNING: Silently ignores errors in the callee-expr evaluation
                if (args->GetSize() != 1)
                    throw mal_error{"MacroExpand takes 1 argument"};
                MalAtom sub_expr(args->First());
                while (mh::is_flist(sub_expr.v)) {
                    MalAtom s_ev_func;
                    try {
                        s_ev_func = EvaluateExpression(sub_expr->li->First(), env);
                    } catch (const mal::mal_error&) {
                        break;
                    }
                    if (s_ev_func->tag == Function_T && s_ev_func->fun->kind == mal::MalFunction::KMacro) {
                        // Macro call
                        sub_expr = EvalFunction(*s_ev_func->fun, sub_expr->li->Rest());
                    } else break;
                }
                RET_VALUE(sub_expr.get());
            }
            else if (sym == "try*") {
                if (args->GetSize() != 3)
                    throw mal_error{"try* takes 3 arguments"};
                if (!mh::is_symbol(args->At(1)))
                    throw mal_error{"Second argument must be a name"};
                try {
                    RET_VALUE(EvaluateExpression(args->First(), env));
                } catch (const mal_error& err) {
                    auto e = std::make_shared<Environment>(env);
                    e->set(args->At(1).st->Get(), err.msg);
                    RET_TCO(args->At(2), e);
                }
            }
        }

        // Call function
        MalValue ev_func = EvaluateExpression(func, env);
        if (!mh::is_invokable(ev_func))
            throw mal_error{"Cannot call non-function"};
        if (ev_func.tag == Function_T && ev_func.fun->kind == mal::MalFunction::KMacro) {
            // MacroExpand
            // In-place macro expansion
            RET_TCO(EvalFunction(*ev_func.fun, args), env);
        }
        auto ev_args = EvalAst(mh::list(args), env).li;
        if (ev_func.tag == Builtin_T)
            RET_VALUE(ev_func.blt(*this, ev_args));
        else /*if (ev_func.tag == Function_T)*/ {
            auto& fun = *ev_func.fun;
            auto n_env = PrepareFunctionCall(fun, ev_args);
            RET_TCO(fun.body, n_env);
        }
    }

    MalValue Interpreter::EvalFunction(const MalFunction& func, MalArgs&& args) {
        return EvaluateExpression(func.body, PrepareFunctionCall(func, std::move(args)));
    }

    MalValue Interpreter::EvaluateExpression(const MalValue& expr, EnvironFrame env) {
        RecursionGuard<MAX_RECURSION_DEPTH> rg{recursion_depth};
        MalAtom curr{expr};
        while (true) {
            switch (curr->tag) {
                case List_T: {
                    if (curr->li == nullptr)
                        return curr.get();
                    if (Apply(curr, env, curr->li->First(), curr->li->Rest()))
                        return std::move(curr.v);
                    break;
                }
                default:
                    return EvalAst(curr.v, env);
            }
        }
    }
}
