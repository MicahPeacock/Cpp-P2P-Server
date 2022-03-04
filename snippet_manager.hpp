#ifndef SNIPPET_MANAGER_HPP
#define SNIPPET_MANAGER_HPP

#include "io_context.hpp"

#include <iostream>
#include <memory>
#include <thread>

class snippet_manager : public std::enable_shared_from_this<snippet_manager> {
public:
    explicit snippet_manager(net::io_context& ioc)
            : m_ioc(ioc) {}

    void run(std::istream& in = std::cin, std::ostream& out = std::cout) {
        std::thread([self = shared_from_this(), &in](){
            self->read(in);
        }).detach();
        std::thread([self = shared_from_this(), &out](){
            self->write(out);
        }).detach();
    }

private:
    void read(std::istream& in) {
        std::string message;
        while(is_running()) {
            std::getline(in, message);
            if(message == "close")
                close();
            else
                m_ioc.put_outgoing(message);
            message.clear();
        }
    }

    void write(std::ostream& out) {
        while(is_running()) {
            if(m_ioc.has_incoming())
                out << m_ioc.pop_incoming() << std::endl;
        }
    }

    bool is_running() const noexcept {
        return m_is_running;
    }

    void close() {
        m_is_running = false;
    }

    net::io_context& m_ioc;
    bool m_is_running = true;
};

#endif //SNIPPET_MANAGER_HPP
