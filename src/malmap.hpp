#pragma once

#include "malvalue.hpp"

#include <unordered_map>
#include <stack>

namespace mal {
    struct MalHash {
        // Implemented in core_lib.cpp
        std::size_t operator ()(const MalValue&) const;
    };
    struct MapSpec;

    // A hash-map type [Map_T]
    struct MalMap {
        std::unordered_map<MalValue, MalAtom, MalHash> data;

        MalMap() {}
        explicit MalMap(const MapSpec& spec);

        auto Lookup(const MalValue& key) {
            return data.find(key);
        }

        void Set(const MalValue& key, const MalValue& value) {
            data[key] = value;
        }

        static std::shared_ptr<MalMap> Make(const MapSpec& spec) {
            return std::make_shared<MalMap>(spec);
        }

        static std::shared_ptr<MalMap> Make() {
            return std::make_shared<MalMap>();
        }
    };

    // A hash-map specialisation type [MapSpec_T]
    // This is a type intended to optimize map construction
    // via (assoc) and (dissoc) forms, the MapSpec value is
    // converted to MalMap on use. This might be the only case
    // when a mal value is mutated
    struct MapSpec {
        union {
            std::shared_ptr<MalMap> v_map;
            std::shared_ptr<MapSpec> v_spec; // May be null
        };

        MalValue key;
        MalValue value; // May be `nil` if unused

        bool s_map; // This spec extends a map (as opposed to another spec)
        bool s_assoc; // This spec specifies association (as opposed to erasure)

        MapSpec(const MalValue& key, const MalValue& value) : key{key}, value{value} {}
        ~MapSpec() {
            if (s_map)
                v_map.~shared_ptr();
            else
                v_spec.~shared_ptr();
        }

        static std::shared_ptr<MapSpec> Make(const std::shared_ptr<MalMap>& map, const MalValue& key, const MalValue& value) {
            auto m = std::make_shared<MapSpec>(key, value);
            new (&m->v_map) std::shared_ptr<MalMap>(map);
            m->s_map = true;
            return m;
        }

        static std::shared_ptr<MapSpec> Make(const std::shared_ptr<MapSpec>& spec, const MalValue& key, const MalValue& value) {
            auto m = std::make_shared<MapSpec>(key, value);
            new (&m->v_spec) std::shared_ptr<MapSpec>(spec);
            m->s_map = false;
            return m;
        }
    };

    inline MalMap::MalMap(const MapSpec& spec) {
        std::stack<const MapSpec*> st;
        const MapSpec* sp = &spec;
        do {
            st.push(sp);
            if (sp->s_map) {
                data = sp->v_map->data; // Copy underlying map
                break;
            }
            sp = sp->v_spec.get();
        } while (sp != nullptr);
        while (!st.empty()) {
            const auto* sc = st.top();
            if (sc->s_assoc) {
                // Insert a key (or overwrite it)
                data[sc->key] = sc->value;
            } else {
                // Erase a key (if it exists)
                data.erase(sc->key);
            }

            st.pop();
        }
    }
}