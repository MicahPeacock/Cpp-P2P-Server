#ifndef SNIPPET_MANAGER_HPP
#define SNIPPET_MANAGER_HPP

#include "io_context.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <utility>

class snippet_manager : public std::enable_shared_from_this<snippet_manager> {
public:
    explicit snippet_manager(net::io_context& ioc)
            : m_ioc(ioc), m_running(false) {}

    /**
     *
     * @param in
     * @param out
     */
    void run(std::istream& in = std::cin, std::ostream& out = std::cout) {
        m_running = true;
        std::thread([self = shared_from_this(), &in](){
            self->read(in);
        }).detach();
        std::thread([self = shared_from_this(), &out](){
            self->write(out);
        }).detach();
    }

    /**
     *
     * @return
     */
    bool is_running() const noexcept {
        return m_running;
    }

    /**
     *
     */
    void close() {
        std::cin.clear(std::ios::eofbit);
        m_running = false;
    }

private:
    /**
     *
     * @param in
     */
    void read(std::istream& in) {
        std::string message;
        while(this->is_running()) {
            std::getline(in, message);
            m_ioc.put_outgoing(message);
            message.clear();
        }
    }

    /**
     *
     * @param out
     */
    void write(std::ostream& out) {
        while(this->is_running()) {
            if(m_ioc.has_incoming())
                out << m_ioc.pop_incoming() << std::endl;
        }
    }

    net::io_context& m_ioc;

    std::atomic<bool> m_running;
};

#endif //SNIPPET_MANAGER_HPP
