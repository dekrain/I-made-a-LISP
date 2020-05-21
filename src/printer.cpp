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
                    operator<<(list->First());
                    first = false;
                }
                stream << (value.tag == List_T ? ')' : ']');
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