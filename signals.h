#pragma once

#include <functional>
#include "intrusive_list.h"

namespace signals {

    template<typename T>
    struct signal;

    template<typename... Args>
    struct signal<void(Args...)> {
    public:
        using slot_t = std::function<void(Args...)>;
        struct connection;
        struct connection_tag;
        using connections_t = intrusive::list<connection, connection_tag>;

        struct iteration_token {
        private:
            signal const *sig = nullptr;
            iteration_token *next = nullptr;
            typename connections_t::const_iterator current;
            friend struct signal;
        public:
            iteration_token(const iteration_token &) = delete;

            iteration_token &operator=(const iteration_token &) = delete;

            explicit iteration_token(signal const *other) : sig(other), current(sig->connections.begin()),
                                                            next(sig->top_token) {
                // current = sig->connections.begin();
                //   next = sig->top_token;// todo какого фига(((
             //   std::cout << alignof(sig) << alignof(next) << alignof(current);
                sig->top_token = this;
            }

            ~iteration_token() {
                if (sig != nullptr) {
                    sig->top_token = next;
                }
            }

        };

        struct connection : intrusive::list_element<connection_tag> {
        public:
            connection() = default;

            connection(connection &&other) noexcept: slot(std::move(other.slot)), sig(other.sig) {
                mover(other);
            }

            connection &operator=(connection &&other) noexcept {
                if (this != &other) {
                    disconnect();
                    slot = std::move(other.slot);
                    sig = other.sig;
                    mover(other);
                }
                return *this;
            };

            connection(signal *sig, slot_t &&slot) : slot(std::move(slot)), sig(sig) {
                sig->connections.push_front(*this);
            };

            ~connection() {
                disconnect();
            }

            void mover(connection &other) {
                if (sig != nullptr) {
                    sig->connections.insert(sig->connections.as_iterator(other), *this);
                    other.unlink();
                    for (iteration_token *tok = sig->top_token; tok; tok = tok->next) {
                        if (tok->current != sig->connections.end() && &*tok->current == &other) {
                            tok->current = sig->connections.as_iterator(*this);
                        }
                    }
                }
            }

            void disconnect() {
                if (this->is_linked() && sig) {
                    this->unlink();
                    slot = {};
                    for (iteration_token *tok = sig->top_token; tok; tok = tok->next) {
                        if (&*tok->current == this) {
                            ++tok->current;
                        }
                    }
                    sig = nullptr;
                }
            }

        private:
            slot_t slot;
            signal *sig = nullptr;
            friend struct signal;
        };


        signal() = default;

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        ~signal() {
            for (iteration_token *tok = top_token; tok; tok = tok->next) {
                tok->sig = nullptr;
            }
            while (!connections.empty()) {
                auto cur = connections.begin();
                cur->unlink();
                cur->slot = {};
                cur->sig = nullptr;
            }
        };

        connection connect(slot_t &&slot) noexcept {
            return connection(this, std::move(slot));
        };

        void operator()(Args... args) const {
            iteration_token tok(this);
            while (tok.current != connections.end()) {
                auto copy = tok.current;
                tok.current++;
                copy->slot(args...);
                if (tok.sig == nullptr) {
                    return;
                }
            }
        }

    private:
        connections_t connections;
        mutable iteration_token *top_token = nullptr;
    };

}
