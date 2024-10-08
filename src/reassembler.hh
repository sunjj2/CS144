#pragma once

#include "byte_stream.hh"
#include <map>
#include <unordered_set>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {}
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }
  void recombination ( uint64_t first_index, string& data, map<uint64_t, string>& unassembled) ;
  uint64_t available_capacity_val () const { return output_._capacity(); }
  uint64_t avail_capacity () const { return output_.writer().available_capacity(); }
  bool error1 () {return output_.has_error();}

  ByteStream output_; // Reassembler 将字节写入此 ByteStream
  uint64_t wait_index = 0; // 期望的下一个字节的索引
  uint64_t end_index = 0; // 流的结束索引
  map<uint64_t, std::string> unassembled_segments_{}; // 存储未组装的字节片段
  bool last_segment_received_ = false; // 标志最后一个子字符串是否已经接收

};


