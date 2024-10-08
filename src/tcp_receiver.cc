#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // SYN 包初始化
  if (message.SYN)  {
    start_index = message.seqno.raw_value_;
    get_strat = true;
    ack = message.seqno + message.sequence_length();  // FIN包的ACK更新
    reassembler_.insert(0, message.payload, message.FIN);  // 对SYN进行一次初始化插入    
    return;
  }

  // 未收到 SYN 则返回
  if (!get_strat) return;
  if (message.FIN ) {
    fin_get = true;
    abs_end_index = message.seqno.unwrap(Wrap32(start_index), reassembler_.wait_index) + message.payload.size();
  }

  uint64_t abs_seqno = message.seqno.unwrap(Wrap32(start_index), reassembler_.wait_index);

  uint64_t reassembler_index = abs_seqno- 1;
  reassembler_.insert(reassembler_index , message.payload, message.FIN);
  if (abs_end_index -1 == reassembler_.wait_index) {
    ack.emplace( Wrap32::wrap(reassembler_.wait_index + 2, Wrap32(start_index)));
  }//
  else 
    ack.emplace( Wrap32::wrap(reassembler_.wait_index + 1, Wrap32(start_index)));
  
  RST1 = message.RST;
}


TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage smessage;
  smessage.ackno = ack;
  smessage.RST = RST1;

  smessage.window_size = reassembler_.avail_capacity() <= UINT16_MAX ? reassembler_.avail_capacity() : UINT16_MAX;
  return smessage;
}

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