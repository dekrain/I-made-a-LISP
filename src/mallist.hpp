#pragma once

#include "malvalue.hpp"

namespace mal {
    class MalList {
        MalValue node;
        std::shared_ptr<MalList> next;

        friend class MalValue;
    public:
        explicit MalList(MalValue&& val) : node{std::move(val)} {}

        const MalValue& First() const {return node; }
        std::shared_ptr<MalList> Rest() const {return next; }

        std::size_t GetSize() const {
            const MalList* p = this;
            std::size_t sz;
            for (sz = 0; p != nullptr; sz++) p = p->Rest().get();
            return sz;
        }

        const MalValue& At(std::size_t idx) const {
            const MalList* p = this;
            for (; p != nullptr && idx > 0; idx--) p = p->Rest().get();
            if (p == nullptr)
                return mh::nil;
            return p->First();
        }

        static std::shared_ptr<MalList> Make(MalValue&& val, std::shared_ptr<MalList> next = nullptr) {
            auto list = std::make_shared<MalList>(std::move(val));
            list->next = next;
            return list;
        }

        friend class ListBuilder;
    };

    class ListIterator {
        std::shared_ptr<MalList> val;
    public:
        ListIterator(std::shared_ptr<MalList> value) : val{std::move(value)} {}

        ListIterator& operator++() {
            if (val)
                val = val->Rest();
            return *this;
        }

        MalValue operator*() {
            if (val)
                return val->First();
            return mh::nil;
        }

        explicit operator bool() const {
            return val != nullptr;
        }
    };

    // Utility used to create Mal Linked Lists
    // Ensure that ListBuilder has the only references to the list, because list are immutable
    class ListBuilder {
        std::shared_ptr<MalList> head = nullptr;
        std::shared_ptr<MalList> node = nullptr;

    public:
        ListBuilder(/*std::shared_ptr<MalList> head = nullptr*/) {}

        void push(MalValue&& val) {
            if (head == nullptr) {
                head = node = MalList::Make(std::move(val));
            } else {
                node->next = MalList::Make(std::move(val));
                node = node->next;
            }
        }

        void prepend(MalValue&& val) {
            if (head == nullptr) {
                head = node = MalList::Make(std::move(val));
            } else {
                head = MalList::Make(std::move(val), head);
            }
        }

        std::shared_ptr<MalList> release() {
            node = nullptr;
            return std::move(head);
        }
    };
}
