#pragma once

#include <vector>
#include "malvalue.hpp"
#include "interpreter.hpp"

namespace mal {
    enum class toktype {
        special = 0, // Things like '(', '}', '~@', '@', etc.
        symbol,
        keyword,
        string,
        number
    };

    // Reader token
    struct token {
        toktype tag;
        std::string val;

        inline token(toktype tag, std::string&& val) : tag{tag}, val{std::move(val)} {}

        inline bool operator==(const token& t) const {
            return tag == t.tag && val == t.val;
        }
    };

    class Reader {
        StringInternPool* str_interner;

        std::vector<token> tokens;
        std::size_t idx = 0;

        std::shared_ptr<MalList> ReadList(const token& endtok);
        MalValue ReadSingle(const token& token);
    public:
        Reader(std::vector<token>&& tokens, StringInternPool* str_interner = nullptr) : str_interner{str_interner}, tokens{tokens} {}

        const token& Peek() const {
            if (idx >= tokens.size())
                throw mal_error{"Unexpected end of token stream"};
            return tokens[idx];
        }

        const token& Next() {
            if (idx >= tokens.size())
                throw mal_error{"Unexpected end of token stream"};
            return tokens[idx++];
        }

        void Skip() {
            ++idx;
        }

        bool IsDrained() const {
            return idx >= tokens.size();
        }

        const std::vector<token>& _get_tokens() const {
            return tokens;
        }

        MalValue ReadForm();

        // TODO(Maybe?): Create internal string class: ::mal::string
        static Reader ParseString(const std::string& src, StringInternPool* str_interner = nullptr);
    };

    static inline MalValue ReadForm(const std::string& src, StringInternPool* str_interner = nullptr) {
        Reader reader = Reader::ParseString(src, str_interner);
        return reader.ReadForm();
    }
}