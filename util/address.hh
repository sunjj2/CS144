#pragma once

#include <cstddef>
#include <cstdint>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>

//! Wrapper around [IPv4 addresses](@ref man7::ip) and DNS operations.封装[IPv4地址](@ref man7::ip)和DNS操作
class Address
{
public:
  //! \brief Wrapper around [sockaddr_storage](@ref man7::socket).
  //! \details A `sockaddr_storage` is enough space to store any socket address (IPv4 or IPv6).
  // !\brief包装围绕[sockaddr_storage](@ref man7::socket)。
  // !' sockaddr_storage '是足够的空间来存储任何套接字地址(IPv4或IPv6)。
  class Raw
  {
  public:
    sockaddr_storage storage {}; //!< The wrapped struct itself.用来储存地址信息
    // NOLINTBEGIN (*-explicit-*)
    operator sockaddr*();
    operator const sockaddr*() const;
    // NOLINTEND (*-explicit-*)
  };

private:
  socklen_t _size; //!< Size of the wrapped address.
  Raw _address {}; //!< A wrapped [sockaddr_storage](@ref man7::socket) containing the address.

  //! 从 IP 地址或主机名 (node)、服务名 (service) 和 DNS 解析器的提示 (hints) 构造 Address 对象。这个构造函数通常用于高级别的地址解析操作
  Address( const std::string& node, const std::string& service, const addrinfo& hints );

public:
  //! 通过解析主机名和服务名构造 Address 对象。
  Address( const std::string& hostname, const std::string& service );

  //! 通过点分十进制字符串（如 "18.243.0.1"）和端口号构造 Address 对象。
  explicit Address( const std::string& ip, std::uint16_t port = 0 );

  //! 从一个 sockaddr* 指针和其大小构造 Address 对象
  Address( const sockaddr* addr, std::size_t size );

  //! Equality comparison.
  bool operator==( const Address& other ) const;
  bool operator!=( const Address& other ) const { return not operator==( other ); }

  //! \name Conversions
  //!@{

  //! Dotted-quad IP address string ("18.243.0.1") and numeric port.返回一个包含点分十进制 IP 地址字符串和端口号的 std::pair
  std::pair<std::string, uint16_t> ip_port() const;
  //! Dotted-quad IP address string ("18.243.0.1").返回点分十进制 IP 地址字符串
  std::string ip() const { return ip_port().first; }
  //! Numeric port (host byte order).返回端口号
  uint16_t port() const { return ip_port().second; }
  //! Numeric IP address as an integer (i.e., in [host byte order](\ref man3::byteorder)).返回 IP 地址的整数表示（主机字节序）
  uint32_t ipv4_numeric() const;//四个数转成二进制然后穿起来
  //! Create an Address from a 32-bit raw numeric IP address从一个 32 位的原始数字 IP 地址创建一个 Address 对象
  static Address from_ipv4_numeric( uint32_t ip_address );
  //! Human-readable string, e.g., "8.8.8.8:53".返回一个人类可读的字符串表示，例如 "8.8.8.8:53"
  std::string to_string() const;
  //!@}

  //! \name Low-level operations
  //!@{

  //! Size of the underlying address storage.返回地址存储的大小。
  socklen_t size() const { return _size; }
  //! Const pointer to the underlying socket address storage.返回底层套接字地址存储的常量指针。
  const sockaddr* raw() const { return static_cast<const sockaddr*>( _address ); }
  //! Safely convert to underlying sockaddr type将地址安全地转换为特定的 sockaddr 类型（例如 sockaddr_in）。
  template<typename sockaddr_type>
  const sockaddr_type* as() const;

  //!@}
};
