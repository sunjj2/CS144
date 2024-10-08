#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffut(){}

bool Writer::is_closed() const
{
  // Your code here.
  return writeclosed;
}

void Writer::push( string data )
{
  if (is_closed()) return;
  size_t canpush_lenth = available_capacity();
  size_t canwrite_lenth = min (data.size(), canpush_lenth);
  buffut.insert(buffut.end(), data.begin(), data.begin() + canwrite_lenth);
  write_lenth += canwrite_lenth;
  return;
}

void Writer::close()
{
  writeclosed = true;
  // Your code here.
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffut.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return write_lenth;
}

bool Reader::is_finished() const
{
  if (writeclosed && buffut.size()==0) {
    return true;
  }
  return false;
}

uint64_t Reader::bytes_popped() const
{
  return read_lenth;
}

string_view Reader::peek() const
{
  if (buffut.empty()) {
    return std::string_view();  // 返回空视图
  }
  return string_view(buffut.data(), buffut.size());
}


void Reader::pop( uint64_t len )
{
  size_t pop_lenth = min(len, buffut.size());
  buffut.erase(buffut.begin(), buffut.begin() + pop_lenth);
  read_lenth += pop_lenth;

}

string Reader::read( uint64_t len )
{
  size_t pop_lenth = min(len, buffut.size());
  string s = buffut.substr(0, pop_lenth);
  pop( len );
  return s;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffut.size();
}
