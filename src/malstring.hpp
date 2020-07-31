#pragma once

#include "malvalue.hpp"
#include <string>

#include <unordered_map>

namespace mal {
    class StringInternPool;
    class MalString {
    public:
        typedef std::string string_t;
    private:
        string_t str;
        StringInternPool* pool = nullptr;
    public:
        MalString(const string_t& val) : str{val} {}
        MalString(const string_t&& val, StringInternPool* pool=nullptr) : str{std::move(val)}, pool{pool} {}

        const string_t& Get() const {return str; }

        static std::shared_ptr<MalString> Make(const string_t& val) {
            return std::make_shared<MalString>(val);
        }

        static std::shared_ptr<MalString> Make(const string_t&& val, StringInternPool* pool=nullptr) {
            return std::make_shared<MalString>(std::move(val), pool);
        }

        bool IsInterned(StringInternPool* pool_) {return pool == pool_; }
    };

    // Class for interning strings
    class StringInternPool {
        using element_type = std::shared_ptr<MalString>;
        /*struct StringHash {
            auto operator()(element_type const& v) const {
                return std::hash(v->Get());
            }
        };
        struct StringEq {
            auto operator()(element_type const& a, element_type const& b) const {
                return a->Get() == b->Get();
            }
        };*/
        std::unordered_map<std::string, element_type/*, StringHash, StringEq*/> pool;
    public:
        element_type Intern(std::string&& str) {
            auto it = pool.find(str);
            if (it == pool.end())
                it = pool.emplace(str, MalString::Make(std::move(str), this)).first;
            return it->second;
        }
        element_type Intern(const std::string& str) {
            return Intern(static_cast<std::string>(str));
        }
    };
}