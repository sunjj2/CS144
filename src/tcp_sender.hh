#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>
//跟踪接收器的窗口大小，尽可能填满窗口，发送方应该继续发送段，直到窗口被填满或出站字节流没有更多的可发送
//跟踪哪些片段已发送但尚未被接收方确认-我们称这些片段为“未完成”片段
//如果未完成的段在发送后经过了足够的时间，并且尚未得到确认，则重新发送


class Timer {
  public:  
    void start(uint64_t timeout);
    void update(uint64_t time_elapsed);
    void reset();
    bool active() const{return _active;}
    bool expired() const {return _active && _expored;}
  private:
    bool _active = false;
    bool _expored =false;
    uint64_t _current_time{0};
    uint64_t _timeout{0};
};

class TCPSender 
{
public:

  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  //收到TCPrecever的信息（窗口大小和ack）
  void receive( const TCPReceiverMessage& msg );  

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  queue<TCPSenderMessage> _segments_out{};
  deque<TCPSenderMessage> _RTO_buf{};
  Timer _timer{};
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t _RTO_ms = initial_RTO_ms_;
  uint16_t _window_size{1};
  uint64_t _next_seqno{0};
  uint64_t _makesure_seqno{0};
  uint64_t _retransmission_number{0};
  bool _syn_sent = false;
  bool _fin_sent = false;
  uint64_t zero_index = 0;
};

