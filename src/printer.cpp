#include "printer.hpp"

#include <sstream>

namespace mal {
    Printer& OstreamPrinter::operator<<(print_begin_t beg) { is_raw = beg.print_raw; return *this; }
    Printer& OstreamPrinter::operator<<(print_end_t) { stream << std::endl; return *this; }
    Printer& OstreamPrinter::operator<<(const std::string& str) { stream << str; return *this; }
    
    Printer& OstreamPrinter::operator<<(const MalValue& value) {
        switch (value.tag) {
            case Nil_T:
                stream << "nil";
                break;
            case True_T:
                stream << "true";
                break;
            case False_T:
                stream << "false";
                break;
            case Int_T:
                stream << value.no;
                break;
            case Symbol_T:
                stream << value.st->Get();
                break;
            case Keyword_T:
                stream << ":" << value.st->Get();
                break;
            case String_T:
                if (is_raw)
                    stream << value.st->Get();
                else
                    stream << EscapeString(value.st->Get());
                break;
            case List_T:
            case Vector_T:
            {
                stream << (value.tag == List_T ? '(' : '[');
                bool first = true;
                for (const MalList* list = value.li.get(); list != nullptr; list = list->Rest().get()) {
                    if (!first) {
                        stream << ' ';
                    }
                    first = false;
                    operator<<(list->First());
                }
                stream << (value.tag == List_T ? ')' : ']');
                break;
            }
            case Map_T:
            case MapSpec_T:
            {
                auto map = mh::as_map(value);
                stream << '{';
                bool first = true;
                for (auto it = map->data.begin(); it != map->data.end(); ++it) {
                    if (!first)
                        stream << ' ';
                    first = false;
                    operator<<(it->first);
                    stream << ' ';
                    operator<<(it->second.v);
                }
                stream << '}';
                break;
            }
            case Builtin_T:
                stream << "<builtin-function>";
                break;
            case Function_T:
                stream << "<function>";
                break;
            case Atom_T:
                stream << "<atom ";
                operator<<(value.at->v);
                stream << '>';
                break;
            default:
                stream << "<unknown>";
        }
        return *this;
    }

    Printer& TTYPrinter::operator<<(const MalValue& value) {
        if (is_raw)
            return _Base::operator<<(value);
        switch (value.tag) {
            case Nil_T:
                stream << TTYColors::nil << "nil" << TTYColors::reset;
                break;
            case True_T:
                stream << TTYColors::boolean << "true" << TTYColors::reset;
                break;
            case False_T:
                stream << TTYColors::boolean << "false" << TTYColors::reset;
                break;
            case Int_T:
                stream << TTYColors::number << value.no << TTYColors::reset;
                break;
            case Keyword_T:
                stream << TTYColors::keyword << ":" << value.st->Get() << TTYColors::reset;
                break;
            case String_T:
                //if (is_raw)
                //    stream << value.st->Get();
                //else
                    stream << TTYColors::string << EscapeString(value.st->Get()) << TTYColors::reset;
                break;
            case Symbol_T:
            case List_T:
            case Vector_T:
            case Map_T:
            case MapSpec_T:
            case Builtin_T:
            case Function_T:
            case Atom_T:
            default:
                return _Base::operator<<(value);
        }
        return *this;
    }

    std::string EscapeString(const std::string& str) {
        std::string res;
        std::size_t i = 0;
        while (i < str.length()) {
            std::size_t next = str.find_first_of("\\\n\"", i);
            if (next != i) {
                if (next == str.npos)
                    res += str.substr(i);
                else
                    res += str.substr(i, next-i);
            }
            i = next;
            if (i != str.npos) {
                char ch = str[i++];
                if (ch == '\\')
                    res += "\\\\";
                else if (ch == '\n')
                    res += "\\n";
                else if (ch == '"')
                    res += "\\\"";
            }
        }
        return '"' + res + '"';
    }
}