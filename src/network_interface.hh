#pragma once

#include <queue>
#include <vector>
#include <utility> // For std::pair
#include <unordered_map> // For std::unordered_map
#include "address.hh"
#include "ethernet_frame.hh"
#include "ipv4_datagram.hh"
using namespace std;
//连接IP(因特网层，或网络层)和以太网(网络接入层，或链路层)的“网络接口”。

//该模块是TCP/IP栈的最低层(将IP与较低层网络协议连接起来，例如以太网)。但同样的模块也被反复用作路由器的一部分:
//路由器通常有许多网络接口，路由器的工作是在不同的接口之间路由互联网数据报。

//网络接口将数据报(来自“客户”，例如TCP/IP栈或路由器)转换成以太网帧。为了填充以太网目的地址，
//它查找每个数据报的下一个IP跳的以太网地址，使用[地址解析协议](\ref rfc::rfc826)的请求。在相反的方向上，
//网络接口接受以太网帧，检查它们是否为它准备的，如果是，则处理有效载荷取决于其类型。
//如果它是IPv4数据报，网络接口将它向上传递堆栈。如果是ARP请求或应答，则网络接口处理该帧，并根据需要学习或应答。
class NetworkInterface
{
public:
  // NetworkInterface发送以太网帧的物理输出端口的抽象
  // An abstraction for the physical output port where the NetworkInterface sends Ethernet frames
  class OutputPort
  {
  public:
    virtual void transmit( const NetworkInterface& sender, const EthernetFrame& frame ) = 0;
    virtual ~OutputPort() = default;
  };

// Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer)
// addresses
// 用给定的以太网(网络访问层)和IP(因特网层)地址构造一个网络接口
  NetworkInterface( std::string_view name,
                    std::shared_ptr<OutputPort> port,
                    const EthernetAddress& ethernet_address,
                    const Address& ip_address );

  // Sends an Internet datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination
  // address). Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next
  // hop. Sending is accomplished by calling `transmit()` (a member variable) on the frame.
  //发送一个封装在以太网帧中的因特网数据报(如果它知道以太网的目的地址)。需要使用[ARP](\ref rfc::rfc826)
  //来查找下一跳的以太网目的地址。发送是通过在帧上调用' transmit() '(一个成员变量)来完成的。
  void send_datagram( const InternetDatagram& dgram, const Address& next_hop );

  // Receives an Ethernet frame and responds appropriately.
  // If type is IPv4, pushes the datagram to the datagrams_in queue.
  // If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
  // If type is ARP reply, learn a mapping from the "sender" fields.
  void recv_frame( const EthernetFrame& frame );   //EthernetFrame& frame，以太网包（包含一个头部和数据负载）

  //当时间流逝时周期性调用
  // Called periodically when time elapses
  void tick( size_t ms_since_last_tick );

  // Accessors
  const std::string& name() const { return name_; }
  const OutputPort& output() const { return *port_; }
  OutputPort& output() { return *port_; }
  std::queue<InternetDatagram>& datagrams_received() { return datagrams_received_; }
  EthernetFrame send_datagram(EthernetAddress dst, uint16_t type, const vector<std::string> payload);
  void remove_expired_cache();
  void try_send_waiting(uint32_t new_ip);
  void send_outgoing_frames();
private:
  // Human-readable name of the interface
  std::string name_;//人可读的接口名称

  // The physical output port (+ a helper function `transmit` that uses it to send an Ethernet frame)
  std::shared_ptr<OutputPort> port_;//指向端口的智能指针，/物理输出端口(+一个辅助函数' transmit '，使用它发送以太网帧)
  void transmit( const EthernetFrame& frame ) const { port_->transmit( *this, frame ); }

  // Ethernet (known as hardsware, network-access-layer, or link-layer) address of the interface
  EthernetAddress ethernet_address_;//MAC地址

  // IP (known as internet-layer or network-layer) address of the interface
  Address ip_address_;//ip地址

  // Datagrams that have been received
  std::queue<InternetDatagram> datagrams_received_ {}; //已经收到的数据报文
  unordered_map<uint32_t, pair<EthernetAddress, size_t>> _add_cache{};
  queue<EthernetFrame> _frames_out{};
  size_t _current_time {0};
  unordered_map<uint32_t, size_t> _addr_request_time{};
  vector<pair<uint32_t, InternetDatagram>> _waiting_dgrams{};
};
