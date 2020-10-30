#include "intrusive_list.h"
#include <cassert>

namespace {
    template<typename T>
    void triswap(T &a, T &b, T &c) noexcept {
        T copy = a;
        a = b;
        b = c;
        c = copy;
    }
}

void intrusive::list_element_base::unlink() noexcept {
    if (next != nullptr)
        next->prev = prev;
    if (prev != nullptr)
        prev->next = next;
    prev = next = nullptr;
}

bool intrusive::list_element_base::is_linked() const  noexcept {
    return (next != nullptr || prev != nullptr);
}

void intrusive::list_element_base::try_unlink() noexcept {
    assert((prev == nullptr) == (next == nullptr));
    if (prev)
        unlink();
}

void intrusive::list_element_base::clear() noexcept {
    auto *p = next;
    while (p != this) {
        auto *n = p->next;
        p->prev = nullptr;
        p->next = nullptr;
        p = n;
    }

    prev = this;
    next = this;
}

void intrusive::list_element_base::insert(list_element_base &obj) noexcept {
    obj.next = this;
    obj.prev = prev;
    prev->next = &obj;
    prev = &obj;
}

void intrusive::list_element_base::splice(list_element_base &first, list_element_base &last) noexcept {
    if (&first == &last)
        return;

    triswap(prev->next, first.prev->next, last.prev->next);
    triswap(prev, last.prev, first.prev);
}