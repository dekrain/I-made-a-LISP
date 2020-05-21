#pragma once

#include "malvalue.hpp"

namespace mal {
    struct MalArgs {
        std::vector<MalValue> vec;

        std::size_t size() const { return vec.size(); }

        MalArgs(const std::shared_ptr<MalList>& list) {
            vec.reserve(list->GetSize());
            for (const MalList* l = list.get(); l != nullptr; l = l->Rest().get())
                vec.push_back(l->First());
        }

        MalArgs(std::initializer_list<MalValue> init) : vec{init} {}

        const MalValue& operator[](std::size_t i) const& {
            return vec[i];
        }

        MalValue&& operator[](std::size_t i) && {
            return std::move(vec[i]);
        }

        decltype(vec)::iterator begin() {
            return vec.begin();
        }

        decltype(vec)::iterator end() {
            return vec.end();
        }

        MalArgs& operator=(const MalArgs&) = delete;
        MalArgs& operator=(MalArgs&&) = delete;
    };

    class Environment;
    using EnvironFrame = std::shared_ptr<Environment>;

    struct Environment {
        std::unordered_map<std::string, MalAtom> data;
        EnvironFrame outer;

        Environment(EnvironFrame outer = nullptr) : outer{std::move(outer)} {}

        void set(const std::string& key, MalValue&& value) {
            data[key] = std::move(value);
        }

        void set(const std::string& key, const MalValue& value) {
            data[key] = value;
        }

        MalValue lookup(const std::string& key) {
            for (const Environment* env = this; env != nullptr; env = env->outer.get()) {
                auto entry = env->data.find(key);
                if (entry != env->data.cend()) {
                    return entry->second.get();
                }
            }
            throw mal_error{"Cannot find '" + key + "' in current context"};
        }
    };

    template <std::size_t max_depth>
    struct RecursionGuard {
        std::size_t& guard;
        RecursionGuard(std::size_t& g) : guard(g) {
            if (++guard > max_depth) {
                --guard;
                throw mal_error{"Recursion limit reached"};
            }
        }
        ~RecursionGuard() {
            --guard;
        }
    };
}
