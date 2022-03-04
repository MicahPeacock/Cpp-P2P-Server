#ifndef TCP_HPP
#define TCP_HPP

#include "connector.hpp"
#include "socket_address.hpp"
#include "stream_socket.hpp"

namespace net::tcp {

using socket    = net::stream_socket<net::address_v4>;
using connector = net::connector<net::tcp::socket>;
//using acceptor  = net::acceptor<net::tcp::socket>;

} // net

#endif // TCP_HPP
