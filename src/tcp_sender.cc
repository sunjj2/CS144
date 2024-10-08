#include "tcp_sender.hh"
#include "tcp_config.hh"
using namespace std;

// struct TCPReceiverMessage
// {
//   std::optional<Wrap32> ackno {};
//   uint16_t window_size {};
//   bool RST {};
// };

// struct TCPSenderMessage
// {
//   Wrap32 seqno { 0 };

//   bool SYN {};
//   std::string payload {};
//   bool FIN {};

//   bool RST {};

//   // How many sequence numbers does this segment use?
//   size_t sequence_length() const { return SYN + payload.size() + FIN; }
// };
// using TransmitFunction = std::function<void( const TCPSenderMessage& )>;



void TCPSender::push( const TransmitFunction& transmit )
{
  uint16_t  remain_window_size = _window_size != 0 ? _window_size : 1;
  if (_window_size < sequence_numbers_in_flight()) {
    return;
  }
  remain_window_size= _window_size - sequence_numbers_in_flight();
  while (true) {
    uint16_t seg_size = remain_window_size;
    if (seg_size == 0) break;
    TCPSenderMessage seg;
  
    if (!_syn_sent) {
      seg_size -= 1;
      seg.SYN = 1;
      _syn_sent = true;
    }
    seg.seqno = Wrap32::wrap(_next_seqno, isn_);
    string seg_data = input_.reader().read(min(seg_size, static_cast<uint16_t> (TCPConfig::MAX_PAYLOAD_SIZE)));
    seg_size -= seg_data.size();
    seg.payload =  seg_data;
    if (!_fin_sent && input_.eof() &&seg_size > 0) {
      seg_size -= 1;
      seg.FIN = 1;
      _fin_sent = true;
    }
    seg_size = seg.sequence_length();
    if (seg_size == 0) break;
    _segments_out.emplace(seg);
    transmit(seg);
    _RTO_buf.push_back(seg);
    _next_seqno += seg_size;
    remain_window_size -= seg_size;
    if (!_timer.active()) {
      _timer.start(_RTO_ms);
    }
  }

  
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage seg;
  return seg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  _window_size = msg.window_size;
  uint64_t ack_seqno = Wrap32(msg.ackno.value_or(Wrap32{0})).unwrap(isn_, _next_seqno);
  if (ack_seqno > _next_seqno) return;
  for (auto it = _RTO_buf.begin(); it != _RTO_buf.end();) {
    if (it->seqno.unwrap(isn_, _next_seqno) + it->sequence_length() < ack_seqno) {
      it = _RTO_buf.erase(it);
      _RTO_ms = initial_RTO_ms_;
      _timer.start(_RTO_ms); 
      _retransmission_number = 0;
    }
    else break;
  }
  if (_RTO_buf.empty()) {
    _timer.reset();
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if (_timer.active()) {
    _timer.update(ms_since_last_tick);
  }
  if (_timer.expired()) {
    transmit(_RTO_buf.front());
    _segments_out.emplace(_RTO_buf.front()) ;
    if (_window_size > 0) {
      _retransmission_number++;
      _RTO_ms *= 2;
    }
    _timer.start(_RTO_ms);
  }
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t res = 0;
  for (auto &seg : _RTO_buf) {
    res += seg.sequence_length();
  }
  return res;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return _retransmission_number;
}



void Timer::start (uint64_t timeout) {
  _timeout = timeout;
  _current_time = 0; 
  _active = true;
}

void Timer::update (uint64_t time_elapsed) {
  if (!_active) {
    throw runtime_error("Trying to update an inactive timer, plase active it first");
  }
  _current_time = _current_time + time_elapsed;
  if (_current_time > _timeout) {
    _expored = true;
  }
}

void Timer::reset () {
  _current_time = 0;
  _active = false;
  _expored = false;
}

