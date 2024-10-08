#include "ethernet_header.hh"

#include <iomanip>
#include <sstream>

using namespace std;

//! \returns A string with a textual representation of an Ethernet address
string to_string( const EthernetAddress address )
{
  stringstream ss {};
  for ( size_t index = 0; index < address.size(); index++ ) {
    ss.width( 2 );
    ss << setfill( '0' ) << hex << static_cast<int>( address.at( index ) );
    if ( index != address.size() - 1 ) {
      ss << ":";
    }
  }
  return ss.str();
}

//! \returns A string with the header's contents
string EthernetHeader::to_string() const
{
  stringstream ss {};
  ss << "dst=" << ::to_string( dst );
  ss << " src=" << ::to_string( src );
  ss << " type=";
  switch ( type ) {
    case TYPE_IPv4:
      ss << "IPv4";
      break;
    case TYPE_ARP:
      ss << "ARP";
      break;
    default:
      ss << "[unknown type " << hex << type << "!]";
      break;
  }

  return ss.str();
}

void EthernetHeader::parse( Parser& parser ) //解析出以太网首部的信息，从parser的input中读取
{
  // read destination address
  for ( auto& b : dst ) {
    parser.integer( b );
  }

  // read source address
  for ( auto& b : src ) {
    parser.integer( b );
  }

  // read frame type (e.g. IPv4, ARP, or something else)
  parser.integer( type );
}

void EthernetHeader::serialize( Serializer& serializer ) const // 将以太网首部的成员变量数据写入到Serializer中，
//以便将其转化为二进制格式进行传输
{
  // write destination address
  for ( const auto& b : dst ) {
    serializer.integer( b );
  }

  // write source address
  for ( const auto& b : src ) {
    serializer.integer( b );
  }

  // write frame type (e.g. IPv4, ARP, or something else)
  serializer.integer( type );
}
