#include "reassembler.hh"
using namespace std;
unordered_set<char> shabi;
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
 
  if (is_last_substring) {
    end_index = first_index + data.size();
    last_segment_received_ = true;
  }
    // 处理越界部分，确保我们只保留有效数据
  if (first_index >= wait_index + output_.writer().available_capacity()) {
    return; // 超出 ByteStream 的容量限制，直接丢弃
  }
  if (first_index + data.size() > wait_index + output_.writer().available_capacity()) {
    data = data.substr(0,  wait_index + output_.writer().available_capacity() - first_index);
  }

    // 丢弃不可用部分的前置字节
  if (first_index < wait_index) {
    size_t discard_size = wait_index - first_index;
      if (discard_size >= data.size()) {
          return; // 整个数据被丢弃
      }
      data = data.substr(discard_size, data.size() - discard_size);
      first_index = wait_index;
  }

  recombination(first_index, data, unassembled_segments_);       
    // 重新组装字节流
  while (!unassembled_segments_.empty() && unassembled_segments_.begin()->first == wait_index) {
      const auto& segment = unassembled_segments_.begin();
      size_t writable_size = min(segment->second.size(), output_.writer().available_capacity());
      output_.writer().push(segment->second.substr(0, writable_size));
      wait_index += writable_size;
      unassembled_segments_.erase(segment);
  }

  // 如果最后一个子字符串已经接收且所有字节都已写入，关闭 ByteStream
  if (last_segment_received_ && wait_index == end_index) {
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
    int result = 0;
    for (auto ch : unassembled_segments_) {
      result += ch.second.size();
    }
    return result;
}

void Reassembler::recombination(uint64_t first_index, string& data, map<uint64_t, string>& unassembled) {
  auto ch = unassembled.begin();
  while (ch != unassembled.end()) {
    if (first_index < ch->first) {
      if (first_index + data.size() <= ch->second.size() + ch->first) {
        data = data.substr(0, ch->first - first_index);
        unassembled[first_index] = data;
        break;
      } else {
        auto next_ch = std::next(ch);
        unassembled.erase(ch);
        ch = next_ch;
        unassembled[first_index] = data;
      }
    } else {
      if (first_index + data.size() > ch->second.size() + ch->first && first_index < ch->first + ch->second.size()) {
        data = data.substr(ch->first + ch->second.size() - first_index, first_index + data.size() - ch->first - ch->second.size());
        first_index = ch->first + ch->second.size();
        ch++;
      } else if (first_index + data.size() <= ch->first + ch->second.size()) {
        return;
      } else {
        ch++;
      }
    }
  }
  if (!data.empty()) unassembled[first_index] = data;
}



