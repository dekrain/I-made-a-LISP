#include "interpreter.hpp"
#include "reader.hpp"
#include "interop.hpp"

#include <sstream>

#define DEF_FUNC(name) static MalValue _Core_##name([[gnu::unused]] Interpreter& interp, MalArgs&& args)
#define EXP_FUNC(symbol_name, internal_name) env_global->set(symbol_name, mh::builtin(_Core_##internal_name));
#define CHECK_ARGS(nArgs, name) \
    if (args.size() != nArgs) \
        throw mal_error{name " takes " #nArgs " argument(s)"}

namespace mal {
    // Checks list equality ignoring list types
    bool ListEqual(const MalValue& a, const MalValue& b);
}

// Feature switches
#define ENABLE_FS 1

#if (ENABLE_FS)
#include <fstream>
#endif

namespace {
    using namespace mal;

    // Arithmetic
    DEF_FUNC(Add) {
        //(void)interp;
        int val = 0;
        for (auto&& v : args) {
            if (!mh::is_num(v))
                throw mal_error{"Plus takes only number arguments"};
            val += v.no;
        }
        return mh::num(val);
    }

    DEF_FUNC(Sub) {
        (void)interp;
        int val = 0;
        if (args.size() == 0)
            throw mal_error{"Minus takes at least one argument"};
        if (args.size() == 1) {
            const MalValue& v = args[0];
            if (!mh::is_num(v))
                throw mal_error{"Minus takes only number arguments"};
            return mh::num(-v.no);
        }
        bool first = true;
        for (auto&& v : args) {
            if (!mh::is_num(v))
                throw mal_error{"Minus takes only number arguments"};
            if (first) {
                val  = v.no;
                first = false;
            } else
                val -= v.no;
        }
        return mh::num(val);
    }

    DEF_FUNC(Mul) {
        (void)interp;
        int val = 1;
        for (auto&& v : args) {
            if (!mh::is_num(v))
                throw mal_error{"Star takes only number arguments"};
            val *= v.no;
        }
        return mh::num(val);
    }

    DEF_FUNC(Div) {
        (void)interp;
        int val = 1;
        if (args.size() == 0)
            throw mal_error{"Slash takes at least two arguments"};
        if (args.size() == 1)
            throw mal_error{"Multiplicative opposite is not allowed"};
        bool first = true;
        for (auto&& v : args) {
            if (!mh::is_num(v))
                throw mal_error{"Slash takes only number arguments"};
            if (first) {
                val  = v.no;
                first = false;
            } else {
                if (v.no == 0)
                    throw mal_error{"Division by zero"};
                val /= v.no;
            }
        }
        return mh::num(val);
    }

    // Types
    DEF_FUNC(NewList) {
        (void)interp;
        ListBuilder builder;
        for (auto&& v : args) {
            builder.push(std::move(v));
        }
        return mh::list(builder.release());
    }

    DEF_FUNC(IsList) {
        (void)interp;
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_list(args[0]));
    }

    DEF_FUNC(NewVector) {
        (void)interp;
        ListBuilder builder;
        for (auto&& v : args) {
            builder.push(std::move(v));
        }
        return mh::vector(builder.release());
    }

    DEF_FUNC(IsVector) {
        (void)interp;
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_vector(args[0]));
    }

    DEF_FUNC(NewMap) {
        if (args.size() & 1)
            throw mal_error{"hash-map takes even number of arguments"};
        auto map = MalMap::Make();
        for (std::size_t i = 0; i < args.size(); i += 2) {
            const auto& key = args[i];
            const auto& value = args[i+1];
            map->Set(key, value);
        }
        return mh::hash_map(map);
    }

    DEF_FUNC(IsMap) {
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_map(args[0]));
    }

    DEF_FUNC(IsSequence) {
        (void)interp;
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_sequence(args[0]));
    }

    DEF_FUNC(NewAtom) {
        if (args.size() == 0)
            return mh::atom();
        return mh::atom(args[0]);
    }

    DEF_FUNC(IsAtom) {
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_atom(args[0]));
    }

    DEF_FUNC(NewSymbol) {
        if (args.size() != 1 || !mh::is_string(args[0]))
            throw mal_error{"symbol: First argument must be a string"};
        return MalValue(args[0].st, Symbol_T);
    }

    DEF_FUNC(IsSymbol) {
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_symbol(args[0]));
    }

    DEF_FUNC(IsString) {
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_string(args[0]));
    }

    DEF_FUNC(NewKeyword) {
        if (args.size() != 1 || !mh::is_string(args[0]))
            throw mal_error{"keyword: First argument must be a string"};
        return MalValue(args[0].st, Keyword_T);
    }

    DEF_FUNC(IsKeyword) {
        if (args.size() == 0)
            return mh::mal_false;
        return mh::bool_val(mh::is_keyword(args[0]));
    }

    // Atoms
    DEF_FUNC(Deref) {
        if (args.size() != 1)
            throw mal_error{"deref takes 1 argument"};
        if (!mh::is_atom(args[0]))
            return mh::nil;
        return args[0].at->get();
    }

    // ! TODO: Check for cycles
    DEF_FUNC(RefSet) {
        if (args.size() != 2)
            throw mal_error{"reset! takes 2 arguments"};
        if (!mh::is_atom(args[0]))
            throw mal_error{"First argument must be an atom"};
        *args[0].at = args[1];
        return args[0].at->get();
    }

    // Lists
    DEF_FUNC(IsEmpty) {
        (void)interp;
        if (args.size() == 0)
            return mh::mal_false;
        const MalValue& v = args[0];
        return mh::bool_val((mh::is_list(v) || mh::is_vector(v)) && (v.li == nullptr));
    }

    DEF_FUNC(ElementCount) {
        (void)interp;
        if (args.size() == 0)
            return mh::nil;
        const MalValue& v = args[0];
        if (!(mh::is_list(v) || mh::is_vector(v)))
            return mh::nil;
        return mh::num(v.li == nullptr ? 0 : v.li->GetSize());
    }

    DEF_FUNC(First) {
        CHECK_ARGS(1, "first");
        if (!mh::is_fseq(args[0]))
            return mh::nil;
        return args[0].li->First();
    }

    DEF_FUNC(Rest) {
        CHECK_ARGS(1, "rest");
        if (!mh::is_fseq(args[0]))
            return mh::nil;
        return mh::list(args[0].li->Rest());
    }

    DEF_FUNC(GetElement) {
        CHECK_ARGS(2, "nth");
        if (!mh::is_num(args[1]))
            throw mal_error{"Second argument must be a valid index"};
        if (!mh::is_fseq(args[0]))
            return mh::nil;
        int idx = args[1].no;
        if (idx < 0 || idx >= args[0].li->GetSize())
            return mh::nil;
        return args[0].li->At(idx);
    }

    DEF_FUNC(Cons) {
        CHECK_ARGS(2, "cons");
        if (!mh::is_list(args[1]) && !mh::is_nil(args[1]))
            throw mal_error{"Second arguemnt must be a list or nil"};
        return mh::list(
            mh::cons(MalValue(args[0]), mh::is_list(args[1]) ? args[1].li : nullptr)
        );
    }

    DEF_FUNC(Concat) {
        mal::ListBuilder lb;
        for (const auto& l : args) {
            if (!mh::is_sequence(l))
                throw mal_error{"All arguments must be lists or vectors"};
            mal::ListIterator it{l.li};
            while (it) {
                lb.push(*it);
                ++it;
            }
        }
        return mh::list(lb.release());
    }

    // Hash maps
    DEF_FUNC(MapAssoc) {
        CHECK_ARGS(3, "assoc");
        if (!mh::is_map(args[0]))
            throw mal_error{"First argument must be a hash-map"};
        return mh::assoc(args[0], args[1], args[2]);
    }

    DEF_FUNC(MapDissoc) {
        CHECK_ARGS(2, "dissoc");
        if (!mh::is_map(args[0]))
            throw mal_error{"First argument must be a hash-map"};
        return mh::dissoc(args[0], args[1]);
    }

    DEF_FUNC(MapGet) {
        CHECK_ARGS(2, "get");
        if (!mh::is_map(args[0]))
            throw mal_error{"First argument must be a hash-map"};
        auto m = mh::as_map(args[0]);
        auto i = m->Lookup(args[1]);
        if (i == m->data.end())
            return mh::nil;
        return i->second.get();
    }

    DEF_FUNC(MapContains) {
        CHECK_ARGS(2, "contains?");
        if (!mh::is_map(args[0]))
            throw mal_error{"First argument must be a hash-map"};
        auto m = mh::as_map(args[0]);
        auto i = m->Lookup(args[1]);
        return mh::bool_val(i != m->data.end());
    }

    DEF_FUNC(MapKeys) {
        CHECK_ARGS(1, "keys");
        if (!mh::is_map(args[0]))
            throw mal_error{"First argument must be a hash-map"};
        auto m = mh::as_map(args[0]);
        ListBuilder lb;
        for (auto it = m->data.begin(); it != m->data.end(); ++it) {
            lb.push(mh::copy(it->first));
        }
        return mh::list(lb.release());
    }

    DEF_FUNC(MapValues) {
        CHECK_ARGS(1, "vals");
        if (!mh::is_map(args[0]))
            throw mal_error{"First argument must be a hash-map"};
        auto m = mh::as_map(args[0]);
        ListBuilder lb;
        for (auto it = m->data.begin(); it != m->data.end(); ++it) {
            lb.push(it->second.get());
        }
        return mh::list(lb.release());
    }

    // Comparisons
    DEF_FUNC(IsEqual) {
        (void)interp;
        if (args.size() != 2)
            throw mal_error{"Equal takes 2 arguments"};
        return mh::bool_val(args[0] == args[1]);
    }

    DEF_FUNC(EqList) {
        (void)interp;
        if (args.size() != 2)
            throw mal_error{"List-Equal takes 2 arguments"};
        return mh::bool_val(ListEqual(args[0], args[1]));
    }

    DEF_FUNC(CmpLT) {
        (void)interp;
        if (args.size() != 2)
            throw mal_error{"LessThan takes 2 arguments"};
        return mh::bool_val(compare(args[0], args[1]) < 0);
    }

    DEF_FUNC(CmpLE) {
        (void)interp;
        if (args.size() != 2)
            throw mal_error{"LessOrEqual takes 2 arguments"};
        return mh::bool_val(compare(args[0], args[1]) <= 0);
    }

    DEF_FUNC(CmpGT) {
        (void)interp;
        if (args.size() != 2)
            throw mal_error{"GreaterThan takes 2 arguments"};
        return mh::bool_val(compare(args[0], args[1]) > 0);
    }

    DEF_FUNC(CmpGE) {
        (void)interp;
        if (args.size() != 2)
            throw mal_error{"GreaterOrEqual takes 2 arguments"};
        return mh::bool_val(compare(args[0], args[1]) >= 0);
    }

    // Printing
    DEF_FUNC(PFormat) {
        (void)interp;
        std::stringstream str;
        OstreamPrinter printer{str};
        printer << print_begin;
        bool first = true;
        for (auto&& v : args) {
            if (first)
                first = false;
            else
                printer << " ";
            printer << v;
        }
        return mh::string(str.str());
    }

    DEF_FUNC(StrCat) {
        (void)interp;
        std::stringstream str;
        OstreamPrinter printer{str};
        printer << print_begin_raw;
        for (auto&& v : args) {
            printer << v;
        }
        return mh::string(str.str());
    }

    DEF_FUNC(PPrint) {
        interp.printer << print_begin;
        bool first = true;
        for (auto&& v : args) {
            if (first)
                first = false;
            else
                interp.printer << " ";
            interp.printer << v;
        }
        interp.printer << print_end;
        return mh::nil;
    }

    DEF_FUNC(PrintLn) {
        interp.printer << print_begin_raw;
        bool first = true;
        for (auto&& v : args) {
            if (first)
                first = false;
            else
                interp.printer << " ";
            interp.printer << v;
        }
        interp.printer << print_end;
        return mh::nil;
    }

    DEF_FUNC(ReadString) {
        if (args.size() != 1) {
            throw mal_error{"read-string takes 1 argument"};
        }
        if (!mh::is_string(args[0])) {
            throw mal_error{"First argument must be a string"};
        }
        const auto& str = args[0].st->Get();
        return mal::ReadForm(str);
    }

    // Runtime
    DEF_FUNC(DoEval) {
        if (args.size() != 1) {
            throw mal_error{"eval takes 1 argument"};
        }
        return interp.EvaluateExpression(args[0], interp.env_global);
    }

    DEF_FUNC(DoThrow) {
        CHECK_ARGS(1, "throw");
        throw mal_error{MalValue(args[0])};
    }

    DEF_FUNC(Apply) {
        CHECK_ARGS(2, "apply");
        if (!mh::is_invokable(args[0]))
            throw mal_error{"First argument must be a function"};
        if (!mh::is_sequence(args[1]))
            throw mal_error{"Second argument must be an argument list"};
        return interp.InvokeFunction(args[0], args[1].li);
    }

#   if (ENABLE_FS)
    DEF_FUNC(Slurp) {
        if (args.size() != 1) {
            throw mal_error{"slurp takes 1 argument"};
        }
        if (!mh::is_string(args[0])) {
            throw mal_error{"First argument must be a string"};
        }
        const auto& fname = args[0].st->Get();
        std::ifstream file{fname};
        if (!file.good()) {
            throw mal_error{std::string{"Could not open file "} + fname};
        }
        std::istreambuf_iterator<char> it{file};
        return mh::string(std::string(it, std::istreambuf_iterator<char>{}));
    }

    DEF_FUNC(LoadLibrary) {
        if (args.size() != 1 || !mh::is_string(args[0]))
            throw mal_error{"load-library: First argument must be a string"};
        bool status = ::mal::InLoadLibrary(interp, args[0].st->Get().c_str());
        if (!status)
            throw mal_error{"Error while loading a library! Aborted"};
        return mh::nil;
    }
#   endif

    /* TODO: (get-system-info) -> {:recursion_limit @int :fs_enabled @bool} */
}

namespace mal {
    // Checks lists recursively.
    bool check_list(std::shared_ptr<MalList> a_, std::shared_ptr<MalList> b_) {
        ListIterator a{a_}, b{b_};
        while (a && b) {
            if (*a != *b)
                return false;
            ++a;
            ++b;
        }
        return !a && !b;
    }

    // Checks maps recursively
    bool check_map(std::shared_ptr<MalMap> a, std::shared_ptr<MalMap> b) {
        if (a->data.size() != b->data.size())
            return false;
        for (auto it = a->data.begin(); it != a->data.end(); ++it) {
            auto itb = b->Lookup(it->first);
            if (itb == b->data.end() || (it->second.v != itb->second.v))
                return false;
        }
        return true;
    }

    bool operator==(const MalValue& a, const MalValue& b) {
        auto tag = a.tag;
        auto btag = b.tag;
        // A & B must be the same type (!exception: A & B's types are [int | bigint] or [map | mapspec])
        if (tag == Map_T || tag == MapSpec_T) {
            if (btag != Map_T && btag != MapSpec_T)
                return false;
            tag = btag = Map_T;
        }
        if (tag != btag)
            return false;
        if (tag == Int_T)
            return a.no == b.no;
        if (tag == List_T || tag == Vector_T)
            return check_list(a.li, b.li);
        if (tag == Map_T /*|| tag == MapSpec_T*/)
            return check_map(mh::as_map(a), mh::as_map(b));
        if (tag == Symbol_T || tag == Keyword_T || tag == String_T)
            return a.st->Get() == b.st->Get();
        if (tag == Builtin_T) // TODO: Abstract builtin storage
            return a.blt == b.blt;
        if (tag == Function_T)
            return a.fun == b.fun;
        if (tag == Atom_T)
            return a.at == b.at; // Check if point to the same atom, not comparing stored values
        return true;
    }

    std::size_t MalHash::operator()(const MalValue& v) const {
        auto tag = v.tag;
        switch (tag) {
            case Int_T:
                return v.no;
            case List_T:
            case Vector_T:
                // FIXME: Add some algorithm for lists and vectors
                return v.li->GetSize();
            case Map_T:
            case MapSpec_T:
                // Note: Maps are intentionally left out
                return static_cast<std::size_t>(-1);
            case Symbol_T:
            case Keyword_T:
            case String_T:
                // Note: strings, keywords and strings with the same content have the same hash
                return std::hash<std::string>()(v.st->Get());
            case Builtin_T:
                return reinterpret_cast<std::size_t>(reinterpret_cast<void*>(v.blt));
            case Function_T:
            case Atom_T:
            // Note: Atoms can't have unique hashes except addresses because their value can change
            default:
                return static_cast<std::size_t>(-1);
        }
    }

    int compare(const MalValue& a, const MalValue& b) {
        if (!mh::is_num(a) || !mh::is_num(b))
            throw mal_error{"Cannot compare non-numbers"};
        return a.no - b.no;
    }

    bool ListEqual(const MalValue& a, const MalValue& b) {
        if (!mh::is_list(a) && !mh::is_vector(a))
            return a == b;
        if (!mh::is_list(b) && !mh::is_vector(b))
            return false;
        ListIterator ia{a.li}, ib{b.li};
        while (ia && ib) {
            if (!ListEqual(*ia, *ib))
                return false;
            ++ia;
            ++ib;
        }
        return !ia && !ib;
    }

    void Interpreter::InitEnv() {
        env_global = std::make_shared<Environment>();
        
        EXP_FUNC("+", Add)
        EXP_FUNC("-", Sub)
        EXP_FUNC("*", Mul)
        EXP_FUNC("/", Div)
        EXP_FUNC("list", NewList)
        EXP_FUNC("list?", IsList)
        EXP_FUNC("vector", NewVector)
        EXP_FUNC("vector?", IsVector)
        EXP_FUNC("hash-map", NewMap)
        EXP_FUNC("map?", IsMap)
        EXP_FUNC("sequence?", IsSequence)
        EXP_FUNC("atom", NewAtom)
        EXP_FUNC("atom?", IsAtom)
        EXP_FUNC("symbol", NewSymbol)
        EXP_FUNC("symbol?", IsSymbol)
        EXP_FUNC("string?", IsString)
        EXP_FUNC("keyword", NewKeyword)
        EXP_FUNC("keyword?", IsKeyword)
        EXP_FUNC("deref", Deref)
        EXP_FUNC("reset!", RefSet)
        EXP_FUNC("empty?", IsEmpty)
        EXP_FUNC("count", ElementCount)
        EXP_FUNC("first", First)
        EXP_FUNC("rest", Rest)
        EXP_FUNC("nth", GetElement)
        EXP_FUNC("cons", Cons)
        EXP_FUNC("concat", Concat)
        EXP_FUNC("assoc", MapAssoc)
        EXP_FUNC("dissoc", MapDissoc)
        EXP_FUNC("get", MapGet)
        EXP_FUNC("contains?", MapContains)
        EXP_FUNC("keys", MapKeys)
        EXP_FUNC("vals", MapValues)
        EXP_FUNC("=", IsEqual)
        EXP_FUNC("list-equal", EqList)
        EXP_FUNC("<", CmpLT)
        EXP_FUNC("<=", CmpLE)
        EXP_FUNC(">", CmpGT)
        EXP_FUNC(">=", CmpGE)
        EXP_FUNC("pr-str", PFormat)
        EXP_FUNC("str", StrCat)
        EXP_FUNC("prn", PPrint)
        EXP_FUNC("println", PrintLn)
        EXP_FUNC("read-string", ReadString)
        EXP_FUNC("eval", DoEval)
        EXP_FUNC("throw", DoThrow)
        EXP_FUNC("apply", Apply)
#       if (ENABLE_FS)
        EXP_FUNC("slurp", Slurp)
        EXP_FUNC("load-library", LoadLibrary)
#       endif
    }
}