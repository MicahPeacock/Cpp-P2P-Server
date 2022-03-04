#ifndef SNIPPET_MANAGER_HPP
#define SNIPPET_MANAGER_HPP

#include "io_context.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <utility>


/**
 * This is the snippet interface that takes messages from a given input stream and feeds it to the server,
 * as well as takes any incoming messages from the server and print them to the given output stream.
 *
 * By default the streams are stdin and stdout.
 */
class snippet_manager : public std::enable_shared_from_this<snippet_manager> {
public:
    explicit snippet_manager(net::io_context& ioc)
            : m_ioc(ioc), m_running(false) {}

    /**
     * Starts the snippet interface.
     *
     * Note: This method is non-blocking, and will only shutdown once the "close()" method is called.
     * @param in the input stream.
     * @param out the output stream.
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
     * Checks if the snippet interface is currently running.
     * @return true if the interface is active, false otherwise.
     */
    bool is_running() const noexcept {
        return m_running;
    }

    /**
     * Shuts down the snippet interface.
     */
    void close() {
        std::cin.clear(std::ios::eofbit);
        m_running = false;
    }

private:
    /**
     * Reads input from the input stream (delimited by a newline), and queues it in outgoing messages.
     * @param in the input stream.
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
     * Takes any incoming messages and prints them to the output stream.
     * @param out the output stream.
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
