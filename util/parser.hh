#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <deque>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class Parser//解析
{
  class BufferList//缓冲区
  {
    uint64_t size_ {};
    std::deque<std::string> buffer_ {};//字符串双端序列
    uint64_t skip_ {};

  public:
    explicit BufferList( const std::vector<std::string>& buffers )
    {
      for ( const auto& x : buffers ) {
        append( x );
      }
    }

    uint64_t size() const { return size_; }
    uint64_t serialized_length() const { return size(); }  //serialized连载
    bool empty() const { return size_ == 0; }

    std::string_view peek() const//查看缓冲区内的下一元素
    {
      if ( buffer_.empty() ) {
        throw std::runtime_error( "peek on empty BufferList" );
      }
      return std::string_view { buffer_.front() }.substr( skip_ );//从skip_开始到结束的字符串
    }

    void remove_prefix( uint64_t len )//删除缓冲区内len长度的数据
    {
      while ( len and not buffer_.empty() ) {
        const uint64_t to_pop_now = std::min( len, peek().size() );
        skip_ += to_pop_now;
        len -= to_pop_now;
        size_ -= to_pop_now;
        if ( skip_ == buffer_.front().size() ) {
          buffer_.pop_front();
          skip_ = 0;
        }
      }
    }

    void dump_all( std::vector<std::string>& out )//将缓冲区内的所有元素全部推出到输出区（out）
    {
      out.clear();
      if ( empty() ) {
        return;
      }
      std::string first_str = std::move( buffer_.front() );
      if ( skip_ ) {
        first_str = first_str.substr( skip_ );
      }
      out.emplace_back( std::move( first_str ) );
      buffer_.pop_front();
      for ( auto&& x : buffer_ ) {
        out.emplace_back( std::move( x ) );
      }
    }

    void dump_all( std::string& out )//将本来是多个字符串的out转化为一个字符串
    {
      std::vector<std::string> concat;
      dump_all( concat );
      if ( concat.size() == 1 ) {
        out = std::move( concat.front() );
        return;
      }

      out.clear();
      for ( const auto& s : concat ) {
        out.append( s );
      }
    }

    std::vector<std::string_view> buffer() const//将缓冲区的各个字符串建立一个智能指针，返回这个只能指针组
    {
      if ( empty() ) {
        return {};
      }
      std::vector<std::string_view> ret;
      ret.reserve( buffer_.size() );
      auto tmp_skip = skip_;
      for ( const auto& x : buffer_ ) {
        ret.push_back( std::string_view { x }.substr( tmp_skip ) );
        tmp_skip = 0;
      }
      return ret;
    }

    void append( std::string str )
    {
      size_ += str.size();
      buffer_.push_back( std::move( str ) );
    }
  };

  BufferList input_;
  bool error_ {};

  void check_size( const size_t size )
  {
    if ( size > input_.size() ) {
      error_ = true;
    }
  }

public:
  explicit Parser( const std::vector<std::string>& input ) : input_( input ) {}//使用bufferList对Parser进行初始化

  const BufferList& input() const { return input_; }

  bool has_error() const { return error_; }
  void set_error() { error_ = true; }
  void remove_prefix( size_t n ) { input_.remove_prefix( n ); }

  template<std::unsigned_integral T>//T需要是一个无符号整型，包括uint_8,uint_16,uint_32;
  void integer( T& out )  //将二进制的数据转换为原来的
  {
    check_size( sizeof( T ) );//缓冲中的元素大小至少要够一个T才能读数，
    if ( has_error() ) {
      return;
    }

    if constexpr ( sizeof( T ) == 1 ) {
      out = static_cast<uint8_t>( input_.peek().front() );
      input_.remove_prefix( 1 );
      return;
    } else {
      out = static_cast<T>( 0 );
      for ( size_t i = 0; i < sizeof( T ); i++ ) {
        out <<= 8;//左移八位空出来的几位填充为0
        out |= static_cast<uint8_t>( input_.peek().front() );//input_.peek().front() 返回的字节与 out 进行按位或运算，然后赋值回 out。
        input_.remove_prefix( 1 );
      }
    }
  }

  void string( std::span<char> out )
  {
    check_size( out.size() );
    if ( has_error() ) {
      return;
    }

    auto next = out.begin();
    while ( next != out.end() ) {
      const auto view = input_.peek().substr( 0, out.end() - next );
      next = std::copy( view.begin(), view.end(), next );
      input_.remove_prefix( view.size() );
    }
  }

  void all_remaining( std::vector<std::string>& out ) { input_.dump_all( out ); }
  void all_remaining( std::string& out ) { input_.dump_all( out ); }
  std::vector<std::string_view> buffer() const { return input_.buffer(); }
};

class Serializer
{
  std::vector<std::string> output_ {};
  std::string buffer_ {};

public:
  Serializer() = default;
  explicit Serializer( std::string&& buffer ) : buffer_( std::move( buffer ) ) {}

  template<std::unsigned_integral T>
  void integer( const T val )//将原来的值转换为2进制，字节是计算机内存操作的最小单元，可表示 0-255 之间的任意整数。
  {
    constexpr uint64_t len = sizeof( T );

    for ( uint64_t i = 0; i < len; ++i ) {
      const uint8_t byte_val = val >> ( ( len - i - 1 ) * 8 );
      buffer_.push_back( byte_val );//将val中的信息写入buffer_中
    }
  }
// 举例：如果 T 是 uint32_t 类型，且 val = 0x12345678，len 为 4。循环执行如下：
// i = 0 时：val >> (24)，得到最高字节 0x12。
// i = 1 时：val >> (16)，得到次高字节 0x34。
// i = 2 时：val >> (8)，得到次低字节 0x56。
// i = 3 时：val >> (0)，得到最低字节 0x78。
  void buffer( std::string buf )
  {
    flush();
    if ( not buf.empty() ) {
      output_.push_back( std::move( buf ) );
    }
  }

  void buffer( const std::vector<std::string>& bufs )
  {
    for ( const auto& b : bufs ) {
      buffer( b );
    }
  }

  void flush()
  {
    if ( not buffer_.empty() ) {
      output_.emplace_back( std::move( buffer_ ) );
      buffer_.clear();
    }
  }

  const std::vector<std::string>& output()
  {
    flush();
    return output_;
  }
};

// Helper to serialize any object (without constructing a Serializer of the caller's own)
template<class T>
std::vector<std::string> serialize( const T& obj )
{
  Serializer s;
  obj.serialize( s );
  return s.output();
}

// Helper to parse any object (without constructing a Parser of the caller's own). Returns true if successful.
template<class T, typename... Targs>
bool parse( T& obj, const std::vector<std::string>& buffers, Targs&&... Fargs )
{
  Parser p { buffers };
  obj.parse( p, std::forward<Targs>( Fargs )... );
  return not p.has_error();
}
