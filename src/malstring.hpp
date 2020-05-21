#pragma once

#include "malvalue.hpp"
#include <string>

namespace mal {
    class MalString {
    public:
        typedef std::string string_t;
    private:
        string_t str;
    public:
        MalString(const string_t& val) : str{val} {}

        const string_t& Get() const {return str; }

        static std::shared_ptr<MalString> Make(const string_t& val) {
            return std::make_shared<MalString>(val);
        }
    };
}