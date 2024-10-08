#pragma once

#include "parser.hh"

#include <array>
#include <cstdint>
#include <string>

// Helper type for an Ethernet address (an array of six bytes)
using EthernetAddress = std::array<uint8_t, 6>;//<uint8_t, 6> 是模板参数。uint8_t 表示数组元素的类型，
//它是一个无符号8位整数类型，可以表示从 0 到 255 的整数。6 表示数组的大小，即数组中元素的数量。

// Ethernet broadcast address (ff:ff:ff:ff:ff:ff)
constexpr EthernetAddress ETHERNET_BROADCAST = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

// Printable representation of an EthernetAddress
std::string to_string( EthernetAddress address );//将数据包的内容转化为一个字符串

// Ethernet frame header
struct EthernetHeader
{
  static constexpr size_t LENGTH = 14;         //!< Ethernet header length in bytes
  static constexpr uint16_t TYPE_IPv4 = 0x800; //!< Type number for [IPv4](\ref rfc::rfc791)
  static constexpr uint16_t TYPE_ARP = 0x806;  //!< Type number for [ARP](\ref rfc::rfc826)

  EthernetAddress dst;
  EthernetAddress src;
  uint16_t type;

  // Return a string containing a header in human-readable format
  std::string to_string() const;

  void parse( Parser& parser );
  void serialize( Serializer& serializer ) const;
};


