#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )//notnull 是一个常见的防御式编程函数，目的是在程序运行时检查某个指针是否为空
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();
  auto it = _add_cache.find(next_hop_ip);
  if (it != _add_cache.end()) {
    Serializer serializer;
    dgram.serialize(serializer);
    vector<std::string> serialized_data = serializer.output();
    _frames_out.emplace(send_datagram((*it).second.first, EthernetHeader::TYPE_IPv4, serialized_data ));
    send_outgoing_frames();
  }
  else {
    auto it1 = _addr_request_time.find(next_hop_ip);
    if (it1 == _addr_request_time.end() || _current_time - (*it1).second > 5000)   {  //5s以上会重新发送ARP
      ARPMessage msg;
      msg.sender_ethernet_address = ethernet_address_;
      msg.sender_ip_address = ip_address_.ipv4_numeric();
      msg.target_ip_address = next_hop_ip;
      msg.opcode = ARPMessage::OPCODE_REQUEST;
      Serializer serializer;
      msg.serialize(serializer);
      vector<std::string> serialized_data = serializer.output();
      _frames_out.emplace(send_datagram(ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP, serialized_data)) ;
      send_outgoing_frames();
      if (it1 == _addr_request_time.end()) {_addr_request_time.emplace(next_hop_ip, _current_time);}
      else {(*it1).second = _current_time;}     
    }
    _waiting_dgrams.emplace_back(make_pair(next_hop_ip, dgram));
  }
}

EthernetFrame NetworkInterface::send_datagram(EthernetAddress dst, uint16_t type, const vector<std::string> payload) {
  EthernetFrame frame;
  frame.header.src = ethernet_address_;
  frame.header.dst = dst;
  frame.payload = payload;
  frame.header.type = type;
  return frame;
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  auto &header = frame.header;
  if (header.dst == ethernet_address_ || header.dst == ETHERNET_BROADCAST) {
    if (header.type == EthernetHeader::TYPE_IPv4) {
      InternetDatagram dgram;
      Parser parser(frame.payload);
      dgram.parse(parser) ;    
      if (!parser.has_error()) {
        datagrams_received_.emplace(dgram);
      }
    }
    else if (header.type == EthernetHeader::TYPE_ARP) {
      ARPMessage asg;
      Parser parser(frame.payload);
      asg.parse(parser) ;   
      if (parser.has_error()) return;
      if (_add_cache.find(asg.sender_ip_address) == _add_cache.end()) {
        _add_cache[asg.sender_ip_address] = {asg.sender_ethernet_address, _current_time};
      }
      else {
        _add_cache[asg.sender_ip_address].first = asg.sender_ethernet_address;
        _add_cache[asg.sender_ip_address].second = _current_time;
      }
      auto it1 = _addr_request_time.find(asg.sender_ip_address);
      if (it1 != _addr_request_time.end()) {
        _addr_request_time.erase(it1);
      }
      try_send_waiting(asg.sender_ip_address);
      if (asg.opcode == ARPMessage::OPCODE_REQUEST && asg.target_ip_address == ip_address_.ipv4_numeric()) {
        ARPMessage reply_msg;
        reply_msg.sender_ethernet_address = ethernet_address_;
        reply_msg.sender_ip_address = ip_address_.ipv4_numeric();
        reply_msg.target_ethernet_address = asg.sender_ethernet_address;
        reply_msg.target_ip_address = asg.sender_ip_address;
        reply_msg.opcode = ARPMessage::OPCODE_REPLY;
        Serializer serializer;
        reply_msg.serialize(serializer);
        vector<std::string> serialized_data = serializer.output();
        _frames_out.emplace(send_datagram(asg.sender_ethernet_address, EthernetHeader::TYPE_ARP, serialized_data));
        send_outgoing_frames();
      }
    }
  }
}

void NetworkInterface::try_send_waiting(uint32_t new_ip) {
  for (auto it = _waiting_dgrams.begin(); it != _waiting_dgrams.end();) {
    if ((*it).first == new_ip) {
      Serializer serializer;
      (*it).second.serialize(serializer);
      vector<std::string> serialized_data = serializer.output();
      _frames_out.emplace(send_datagram(_add_cache[new_ip].first, EthernetHeader::TYPE_IPv4, serialized_data));
      send_outgoing_frames();
      it = _waiting_dgrams.erase(it); // 正确地移除已发送的数据报文
    } else {
      ++it;
    }
  }
}


void NetworkInterface::send_outgoing_frames() {
    while (!_frames_out.empty()) {
        EthernetFrame frame = std::move(_frames_out.front()); // 取出队列中的第一个帧
        _frames_out.pop(); // 移除已取出的帧

       port_->transmit(*this, frame);
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  _current_time += ms_since_last_tick;
  remove_expired_cache();
}

void NetworkInterface::remove_expired_cache() {
  for (auto it = _add_cache.begin();it != _add_cache.end();) {
    if ( _current_time - (*it).second.second > 30000)  it = _add_cache.erase(it);
    else ++it;
  }
}


