#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  uint32_t nchan = static_cast<uint32_t>((n + zero_point.raw_value_) % (1ULL << 32));
  return Wrap32(nchan);  
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  int32_t interval = raw_value_ - wrap(checkpoint, zero_point).raw_value_;
  int64_t result = interval + checkpoint;
  if (result >= 0) {
    return result;
  }
  else {
    return result + (1ULL << 32);
  }

}
