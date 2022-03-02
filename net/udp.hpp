#ifndef UDP_HPP
#define UDP_HPP

#include "datagram_socket.hpp"
#include "socket_address.hpp"

namespace net::udp {

using socket = datagram_socket<net::address_v4>;

} // net::udp

#endif //UDP_HPP
