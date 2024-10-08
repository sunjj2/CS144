#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  uint32_t route_round = route_prefix & get_mask(prefix_length);  //得到网络地址
  auto iter = _routing_table.find(route_round);
  if (iter == _routing_table.end() || iter->second.prefix_length < prefix_length) {
    _routing_table[route_round] = {prefix_length, next_hop, interface_num};
  }
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.

void Router::route()
{
  for (auto& interfaces_ptr : _interfaces) {
    while (!(interfaces_ptr->datagrams_received().empty())) {
      auto dgram = interfaces_ptr->datagrams_received().front();
      uint32_t target_ip = dgram.header.dst;
      if (dgram.header.ttl <= 1) return;
      bool routed = false;

      for (uint8_t pre_len = 32; pre_len <= 32; --pre_len) {
        uint32_t mask = get_mask(pre_len);
        auto iter = _routing_table.find(target_ip & mask);

        if (iter != _routing_table.end() && iter->second.prefix_length == pre_len) {
          --dgram.header.ttl;
          RouteItem item = iter->second;

          if (item.next_hop.has_value()) {
            interfaces_ptr->send_datagram(dgram, item.next_hop.value());
          } else {
            interfaces_ptr->send_datagram(dgram, Address::from_ipv4_numeric(dgram.header.dst));
          }

          interfaces_ptr->datagrams_received().pop();
          routed = true;
          break;
        }
      }

      if (!routed) {
        cerr << "DEBUG: No route found for datagram with destination IP " << Address::from_ipv4_numeric(dgram.header.dst).ip() << "\n";
        interfaces_ptr->datagrams_received().pop();
      }
    }
  }
}
