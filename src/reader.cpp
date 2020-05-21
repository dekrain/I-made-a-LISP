#include "reader.hpp"

#include <cstring>
#include <stack>

static const char* const mal_single_chars = "[]{}()'`~^@";
static const char* const mal_all_chars = " ,\t\v\f\r\n" "[]{}()'`~^@" ";";

namespace mal {
    void _ReadInt(const std::string& src, std::size_t& i, std::vector<token>& tokens) {
        std::size_t start = i;
        if (src[i] == '+' || src[i] == '-')
            ++i;
        while (i < src.length()) {
            char ch = src[i++];
            if (ch < '0' || ch > '9') {
                if (std::strchr(mal_all_chars, ch)) {
                    --i;
                    break;
                }
                if (ch == '_')
                    continue;
                throw mal_error{"[Syntax Error] Invalid number"};
            }
        }
        tokens.emplace_back(toktype::number, std::string{src, start, i-start});
    }

    int _ParseInt(const std::string& token) {
        bool neg = token[0] == '-';
        std::size_t it = (token[0] == '-' || token[0] == '+') ? 1 : 0;
        int v = 0;
        while (it < token.size()) {
            if (token[it] == '_') {
                ++it;
                continue;
            }
            int d = token[it++] - '0';
            v = v * 10 + d;
        }
        if (neg)
            v = -v;
        return v;
    }

    bool _CheckNumber(char c1, char c2) {
        if (c1 == '+' || c1 == '-')
            return c2 >= '0' && c2 <= '9';
        return c1 >= '0' && c1 <= '9';
    }

    Reader Reader::ParseString(const std::string& src) {
        std::vector<token> tokens;
        for (std::size_t i = 0; i < src.length(); ) {
            char ch = src[i++];
            if (std::isspace(ch) || ch == ',')
                continue;
            if (ch == '~') {
                if (i < src.length() && src[i] == '@') {
                    ++i;
                    tokens.emplace_back(toktype::special, "~@");
                    continue;
                }
            }
            if (std::strchr(mal_single_chars, ch) != nullptr) {
                tokens.emplace_back(toktype::special, std::string{ch});
                continue;
            }
            if (ch == '"') {
                std::string buf;
                bool finish = false;
                while (i < src.length()) {
                    ch = src[i++];
                    if (ch == '"') {
                        finish = true;
                        break;
                    } else if (ch == '\\') {
                        if (i >= src.length())
                            break;
                        ch = src[i++];
                        if (ch == '\\')
                            buf += '\\';
                        else if (ch == '"')
                            buf += '"';
                        else if (ch == 'n')
                            buf += '\n';
                        else
                            buf += ch;
                    } else {
                        buf += ch;
                    }
                }
                if (!finish) {
                    throw mal_error{"[Syntax Error] Incomplete string"};
                }
                tokens.emplace_back(toktype::string, std::move(buf));
                continue;
            }
            if (ch == ';') {
                while (i < src.length() && src[i++] != '\n');
                continue;
            }
            if (ch >= '0' && ch <= '9') {
                _ReadInt(src, --i, tokens);
                continue;
            }
            if (i < src.length() && (ch == '+' || ch == '-')) {
                if (src[i] >= '0' && src[i] <= '9') {
                    _ReadInt(src, --i, tokens);
                    continue;
                }
            }
            std::size_t end = src.find_first_of(mal_all_chars, --i);
            toktype typ = src[i] == ':' ? toktype::keyword : toktype::symbol;
            if (typ == toktype::keyword)
                ++i;
            if (end == src.npos) {
                tokens.emplace_back(typ, std::string{src, i});
            } else {
                tokens.emplace_back(typ, std::string{src, i, end-i});
            }
            i = end;
        }
        return Reader{std::move(tokens)};
    }

    MalValue Reader::ReadForm() {
        const token& tok = Next();
        if (tok.tag == toktype::special) {
            if (tok.val == "(") {
                return mh::list(ReadList(token{toktype::special, ")"}));
            } else if (tok.val == ")") {
                throw mal_error{"Unexpected character while parsing: ')'"};
            } else if (tok.val == "[") {
                return mh::vector(ReadList(token{toktype::special, "]"}));
            } else if (tok.val == "]") {
                throw mal_error{"Unexpected character while parsing: ']'"};
            } else if (tok.val == "@") {
                return mh::list(mh::cons(mh::symbol("deref"), mh::cons(ReadForm(), nullptr)));
            } else if (tok.val == "'") {
                return mh::list(mh::cons(mh::symbol("quote"), mh::cons(ReadForm(), nullptr)));
            } else if (tok.val == "`") {
                return mh::list(mh::cons(mh::symbol("quasiquote"), mh::cons(ReadForm(), nullptr)));
            } else if (tok.val == "~") {
                return mh::list(mh::cons(mh::symbol("unquote"), mh::cons(ReadForm(), nullptr)));
            } else if (tok.val == "~@") {
                return mh::list(mh::cons(mh::symbol("splice-unquote"), mh::cons(ReadForm(), nullptr)));
            } else {
                throw mal_error{"Undefined token: " + tok.val};
            }
        } else {
            return ReadSingle(tok);
        }
    }

    std::shared_ptr<MalList> Reader::ReadList(const token& endtok) {
        ListBuilder list;
        while (true) {
            const token& tok = Peek();
            if (tok == endtok) {
                Next();
                break;
            }
            MalValue val = ReadForm();
            list.push(std::move(val));
        }
        return list.release();
    }

    MalValue Reader::ReadSingle(const token& token) {
        switch (token.tag) {
            case toktype::symbol:
                if (token.val == "nil")
                    return mh::nil;
                else if (token.val == "true")
                    return mh::mal_true;
                else if (token.val == "false")
                    return mh::mal_false;
                else
                    return mh::symbol(token.val);
            case toktype::number:
                return mh::num(_ParseInt(token.val));
            case toktype::keyword:
                return mh::keyword(token.val);
            case toktype::string:
                return mh::string(token.val);
            default:
                throw mal_error{"Undefined token"};
        }
    }
}