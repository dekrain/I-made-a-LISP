#pragma once

#include <memory>
#include <functional>
#include <stack>
#include <vector>
#include <type_traits>

namespace mal {
    class mal_error;
    class MalList;
    class MalMap;
    class MapSpec;
    class MalString;
    class MalFunction;

    enum MalType {
        Nil_T = 0,
        True_T,
        False_T,
        Int_T,
        //Bigint_T // TODO
        List_T,
        Vector_T,
        Map_T,
        MapSpec_T, // Can be converted to map
        Symbol_T,
        Keyword_T,
        String_T,
        Builtin_T,
        Function_T,
        Atom_T,
    };

    struct MalAtom;

    struct RefCounter {
        static std::size_t total_refs;
        RefCounter() {++total_refs; }
        ~RefCounter() {--total_refs; }
    };

    struct MalValue {
        using builtin_t = MalValue(*)(class Interpreter&, class MalArgs&&);

        MalType tag;
        union {
            struct{} nu; // For null initialization
            std::shared_ptr<MalList> li;
            std::shared_ptr<MalMap> mp;
            std::shared_ptr<MapSpec> ms;
            std::shared_ptr<MalString> st;
            builtin_t blt;
            std::shared_ptr<MalFunction> fun;
            std::shared_ptr<MalAtom> at;
            int no;
        };
        std::shared_ptr<MalAtom> meta;

        constexpr MalValue()
            : tag{Nil_T},
              nu{} {}

        constexpr explicit MalValue(bool v)
            : tag{v ? True_T : False_T},
              nu{} {}

        constexpr MalValue(int v)
            : tag{Int_T},
              no{v} {}

        MalValue(std::shared_ptr<MalList> list, MalType tag)
            : tag{tag},
              li{std::move(list)} {}

        MalValue(std::shared_ptr<MalMap> map)
            : tag{Map_T},
              mp{std::move(map)} {}

        MalValue(std::shared_ptr<MapSpec> spec)
            : tag{MapSpec_T},
              ms{std::move(spec)} {}

        MalValue(std::shared_ptr<MalString> str, MalType tag)
            : tag{tag},
              st{std::move(str)} {}

        MalValue(builtin_t builtin)
            : tag{Builtin_T},
              blt{std::move(builtin)} {}

        MalValue(std::shared_ptr<MalFunction> function)
            : tag{Function_T},
              fun{std::move(function)} {}
        
        MalValue(std::shared_ptr<MalAtom> atom)
            : tag{Atom_T},
              at{std::move(atom)} {}

        ~MalValue() {
            switch (tag) {
                case List_T:
                case Vector_T:
                    li.~shared_ptr();
                    break;
                case Map_T:
                    mp.~shared_ptr();
                    break;
                case MapSpec_T:
                    ms.~shared_ptr();
                    break;
                case String_T:
                case Symbol_T:
                case Keyword_T:
                    st.~shared_ptr();
                    break;
                // Reserved for future modifications
                case Builtin_T:
                    break;
                case Function_T:
                    fun.~shared_ptr();
                    break;
                case Atom_T:
                    at.~shared_ptr();
                    break;
                // Noops
                case Nil_T:
                case True_T:
                case False_T:
                case Int_T:
                    break;
            }
        }

        MalValue(const MalValue& cop) : tag{cop.tag}, meta{cop.meta} {
            switch (tag) {
                case List_T:
                case Vector_T:
                    init(li, cop.li);
                    break;
                case Map_T:
                    init(mp, cop.mp);
                    break;
                case MapSpec_T:
                    init(ms, cop.ms);
                    break;
                case String_T:
                case Symbol_T:
                case Keyword_T:
                    init(st, cop.st);
                    break;
                case Builtin_T:
                    init(blt, cop.blt);
                    break;
                case Function_T:
                    init(fun, cop.fun);
                    break;
                case Atom_T:
                    init(at, cop.at);
                    break;
                case Int_T:
                    no = cop.no;
                    break;
                // Noops
                case Nil_T:
                case True_T:
                case False_T:
                    break;
            }
        }

        MalValue(MalValue&& src) : tag{std::exchange(src.tag, Nil_T)}, meta{std::move(src.meta)} {
            switch (tag) {
                case List_T:
                case Vector_T:
                    init(li, std::move(src.li));
                    break;
                case Map_T:
                    init(mp, std::move(src.mp));
                    break;
                case MapSpec_T:
                    init(ms, std::move(src.ms));
                    break;
                case String_T:
                case Symbol_T:
                case Keyword_T:
                    init(st, std::move(src.st));
                    break;
                case Builtin_T:
                    init(blt, std::move(src.blt));
                    break;
                case Function_T:
                    init(fun, std::move(src.fun));
                    break;
                case Atom_T:
                    init(at, std::move(src.at));
                    break;
                case Int_T:
                    no = src.no;
                    break;
                // Noops
                case Nil_T:
                case True_T:
                case False_T:
                    break;
            }
        }

        // MalValue is immutable...
        MalValue& operator=(const MalValue&) = delete;
        MalValue& operator=(MalValue&&) = delete;

        // ...except for this... Yep!
        // Requires either Map_T or MapSpec_T
        std::shared_ptr<MalMap> Map() const {
            if (tag == MapSpec_T) {
                auto& th = const_cast<MalValue&>(*this);
                auto sp = std::move(th.ms);
                th.ms.~shared_ptr();
                th.init(th.mp, std::make_shared<MalMap>(*sp));
                th.tag = Map_T;
            }
            return mp;
        }

        MalValue Meta() const;

        void SetMeta(const MalValue& m);
        void SetMeta(std::nullptr_t) {
            meta = nullptr;
        }

        friend inline bool operator!=(const MalValue& a, const MalValue& b) { return !(a == b); }
        friend bool operator==(const MalValue&, const MalValue&);
        friend int compare(const MalValue& a, const MalValue& b);

    private:
        // A helper to initialize union members
        template <typename T, typename... Args>
        inline void init(T& v, Args&&... args) {
            new (&v) T{std::forward<Args>(args)...};
        }
    };

    bool operator==(const MalValue&, const MalValue&);
    int compare(const MalValue& a, const MalValue& b);

    // A mutable value container
    struct MalAtom {
        union {
            MalValue v;
        };

        constexpr MalAtom() : v{} {}
        MalAtom(MalValue&& val) : v{std::move(val)} {}
        MalAtom(const MalValue& val) : v{val} {}
        MalAtom(const MalAtom& a) : v{a.v} {}

        ~MalAtom() {
            v.~MalValue();
        }

        MalValue get() const {
            return v;
        }

        MalAtom& operator=(const MalValue& val) {
            v.~MalValue();
            init(v, val);
            return *this;
        }

        MalAtom& operator=(MalValue&& val) {
            v.~MalValue();
            init(v, std::move(val));
            return *this;
        }

        const MalValue* operator->() const {
            return &v;
        }

        static std::shared_ptr<MalAtom> Make(const MalValue& val) {
            return std::make_shared<MalAtom>(val);
        }

    private:
        // A helper to initialize union members
        template <typename T, typename... Args>
        inline void init(T& v, Args&&... args) {
            new (&v) T{std::forward<Args>(args)...};
        }
    };

    inline void MalValue::SetMeta(const MalValue& m) {
        meta = MalAtom::Make(m);
    }

    inline MalValue MalValue::Meta() const {
        return meta ? meta->get() : MalValue();
    }
}

namespace mh {
    // Static values

    static const mal::MalValue nil{};
    static const mal::MalValue mal_true{true};
    static const mal::MalValue mal_false{false};
}

#include "malstring.hpp"

namespace mal {
    struct mal_error {
        MalValue msg;

        mal_error(MalValue&& msg) : msg(std::move(msg)) {}
        mal_error(std::string&& s_msg) : msg(mal::MalValue{mal::MalString::Make(std::move(s_msg)), mal::String_T}) {}
    };
}

#include "mallist.hpp"
#include "malmap.hpp"
#include "malfunction.hpp"

// Mal helpers
namespace mh {
    using std::move;
    template <typename T>
    inline T copy(const T& v) {return v; }

    // Value makers

    inline std::shared_ptr<mal::MalString> maybe_intern(std::string&& str, mal::StringInternPool* pool) {
        return pool ? pool->Intern(move(str)) : mal::MalString::Make(move(str));
    }

    inline mal::MalValue list(const std::shared_ptr<mal::MalList>& list) {
        return mal::MalValue{list, mal::List_T};
    }

    inline mal::MalValue vector(const std::shared_ptr<mal::MalList>& list) {
        return mal::MalValue{list, mal::Vector_T};
    }

    inline mal::MalValue hash_map(const std::shared_ptr<mal::MalMap>& map) {
        return mal::MalValue{map};
    }

    inline mal::MalValue symbol(const std::string& str) {
        return mal::MalValue{mal::MalString::Make(str), mal::Symbol_T};
    }

    inline mal::MalValue string(const std::string& str) {
        return mal::MalValue{mal::MalString::Make(str), mal::String_T};
    }

    inline mal::MalValue keyword(const std::string& str) {
        return mal::MalValue{mal::MalString::Make(str), mal::Keyword_T};
    }

    inline mal::MalValue symbol(std::shared_ptr<mal::MalString>&& str) {
        return mal::MalValue{std::move(str), mal::Symbol_T};
    }

    inline mal::MalValue string(std::shared_ptr<mal::MalString>&& str) {
        return mal::MalValue{std::move(str), mal::String_T};
    }

    inline mal::MalValue keyword(std::shared_ptr<mal::MalString>&& str) {
        return mal::MalValue{std::move(str), mal::Keyword_T};
    }

    inline mal::MalValue num(int val) {
        return mal::MalValue{val};
    }

    inline mal::MalValue bool_val(bool val) {
        return mal::MalValue{val};
    }

    /*inline MalValue num(BigInt&& val) {
        return MalValue{val};
    }*/

    inline mal::MalValue builtin(mal::MalValue::builtin_t builtin) {
        return mal::MalValue{builtin};
    }

    inline mal::MalValue atom(const mal::MalValue& val = mh::nil) {
        return mal::MalValue{mal::MalAtom::Make(val)};
    }

    // Type predicates
    constexpr inline bool is_nil(const mal::MalValue& val) {return val.tag == mal::Nil_T; }
    constexpr inline bool is_true(const mal::MalValue& val) {return val.tag == mal::True_T; }
    constexpr inline bool is_false(const mal::MalValue& val) {return val.tag == mal::False_T; }
    constexpr inline bool is_num(const mal::MalValue& val) {return val.tag == mal::Int_T /*|| val.tag == mal::Bigint_T*/; }
    constexpr inline bool is_symbol(const mal::MalValue& val) {return val.tag == mal::Symbol_T; }
    constexpr inline bool is_keyword(const mal::MalValue& val) {return val.tag == mal::Keyword_T; }
    constexpr inline bool is_string(const mal::MalValue& val) {return val.tag == mal::String_T; }
    constexpr inline bool is_list(const mal::MalValue& val) {return val.tag == mal::List_T; }
    constexpr inline bool is_vector(const mal::MalValue& val) {return val.tag == mal::Vector_T; }
    constexpr inline bool is_map(const mal::MalValue& val) {return val.tag == mal::Map_T || val.tag == mal::MapSpec_T; }
    constexpr inline bool is_sequence(const mal::MalValue& val) {return val.tag == mal::List_T || val.tag == mal::Vector_T; }
    constexpr inline bool is_flist(const mal::MalValue& val) {return val.tag == mal::List_T && (val.li != nullptr); } // Is non-empty list?
    constexpr inline bool is_fseq(const mal::MalValue& val) {return is_sequence(val) && (val.li != nullptr); } // Is non-empty collection?
    constexpr inline bool is_atom(const mal::MalValue& val) {return val.tag == mal::Atom_T; }
    constexpr inline bool is_invokable(const mal::MalValue& val) {return val.tag == mal::Builtin_T || val.tag == mal::Function_T; }

    // val must be a map
    inline auto as_map(const mal::MalValue& val) {return val.Map(); }
    inline auto assoc(const mal::MalValue& map, const mal::MalValue& key, const mal::MalValue& val) {
        std::shared_ptr<mal::MapSpec> p;
        if (map.tag == mal::MapSpec_T)
            p = mal::MapSpec::Make(map.ms, key, val);
        else
            p = mal::MapSpec::Make(map.mp, key, val);
        p->s_assoc = true;
        return p;
    }
    inline auto dissoc(const mal::MalValue& map, const mal::MalValue& key) {
        std::shared_ptr<mal::MapSpec> p;
        if (map.tag == mal::MapSpec_T)
            p = mal::MapSpec::Make(map.ms, key, mh::nil);
        else
            p = mal::MapSpec::Make(map.mp, key, mh::nil);
        p->s_assoc = false;
        return p;
    }

    // Helper functions

    inline std::shared_ptr<mal::MalList> cons(mal::MalValue&& car, std::shared_ptr<mal::MalList> cdr = nullptr) {
        return mal::MalList::Make(std::move(car), std::move(cdr));
    }

    template <typename F, typename = std::enable_if_t<std::is_invocable_r_v<mal::MalValue, F, const mal::MalValue&>>>
    inline std::shared_ptr<mal::MalList> map(const mal::MalList& list, /*std::function<mal::MalValue(const mal::MalValue&)>*/ F&& func) {
        mal::ListBuilder l;
        for (const mal::MalList* node = &list; node != nullptr; node = node->Rest().get()) {
            l.push(func(node->First()));
        }
        return l.release();
    }
}
