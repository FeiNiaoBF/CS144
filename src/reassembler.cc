#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  debug( "--- Insert called ---" );
  debug( "Input: index={}, data=\"{}\", is_last={}", first_index, data, is_last_substring );
  debug( "State before: next_index={}, unassembled_bytes={}, available_capacity={}",
         next_index_,
         unassembled_bytes_,
         output_.writer().available_capacity() );

  // EOF handling
  // 对于EOF的处理，
  if ( is_last_substring ) {
    // 标记，不轻易改变
    eof_received_ = true;
    // 计算整个流的结束位置（字节）
    eof_index_ = first_index + data.size();
  }
  // capacity handling
  // 数据检查和修剪
  // available_cap 是 ByteStream 中还剩下多少可用空间，也是整个缓冲区可用的空间
  uint64_t available_cap = output_.writer().available_capacity();
  // => [window_start, window_end)
  uint64_t window_start = next_index_;
  uint64_t window_end = next_index_ + available_cap;
  // 1. 如果数据在左窗口之外
  if ( first_index < window_start ) {
    if ( first_index + data.size() <= window_start ) {
      // 完全在窗口左侧，直接丢弃
      if ( eof_received_ && unassembled_bytes_ == 0 && next_index_ == eof_index_ ) {
        output_.writer().close();
      }
      return;
    } else {
      // 部分在窗口左侧，修剪掉左侧部分
      uint64_t trim_size = window_start - first_index;
      data = data.substr( trim_size );
      first_index = window_start;
    }
  }
  // 2. 如果数据在右窗口之外
  if ( first_index >= window_end ) {
    // 完全在窗口右侧，直接丢弃
    if ( eof_received_ && unassembled_bytes_ == 0 && next_index_ == eof_index_ )
      output_.writer().close();
    return;
  } else if ( first_index + data.size() > window_end ) {
    // 部分在窗口右侧，修剪掉右侧部分
    data = data.substr( 0, window_end - first_index );
  }
  // 3. 如果裁剪后数据为空，则无需继续
  if ( data.empty() ) {
    if ( eof_received_ && unassembled_bytes_ == 0 && next_index_ >= eof_index_ ) {
      output_.writer().close();
    }
    return;
  }
  // 处理重叠和存储
  // 检查新数据是否完全被现有数据包含
  for ( const auto& seg : segments_ ) {
    if ( seg.first <= first_index && seg.first + seg.second.size() >= first_index + data.size() ) {
      // 新数据完全被现有数据包含，无需插入
      debug("New data completely contained in existing segment at index {}", seg.first);
      if ( eof_received_ && unassembled_bytes_ == 0 && next_index_ >= eof_index_ ) {
        output_.writer().close();
      }
      return;
    }
  }

  // 处理与已存储数据块的重叠, 向前合并
  auto it = segments_.lower_bound( first_index );
  if ( it != segments_.begin() ) {
    it--;
    // it 指向第一个起始位置小于等于 first_index 的数据块，即前一块
    if ( it->first + it->second.size() > first_index ) {
      // 部分重叠，向前合并
      debug("Merging with previous segment at index {}", it->first);
      unassembled_bytes_ -= it->second.size();
      uint64_t overlap_size = it->first + it->second.size() - first_index;
      it->second.append( data.substr( overlap_size ) );
      unassembled_bytes_ += it->second.size();
      debug("After merging with previous, first_index={}, data=\"{}\"", it->first, it->second);
      first_index = it->first;
      data = it->second;
    }
  }
  // 2. 继续处理与已存储数据块的重叠, 向后合并
  auto next_it = segments_.upper_bound( first_index );
  while ( next_it != segments_.end() && first_index + data.size() >= next_it->first ) {
    if ( first_index + data.size() < next_it->first + next_it->second.size() ) {
      uint64_t overlap_size = first_index + data.size() - next_it->first;
      data.append( next_it->second.substr( overlap_size ) );
      debug("Merging with next segment at index {}, after merging data=\"{}\"", next_it->first, data);
    }
    debug("Erasing next segment at index {}", next_it->first);
    unassembled_bytes_ -= next_it->second.size();
    next_it = segments_.erase( next_it );
  }

  // 更新map中的数据
  segments_[first_index] = std::move( data );
  unassembled_bytes_ = 0;
  for ( const auto& seg : segments_ ) {
    unassembled_bytes_ += seg.second.size();
    debug("Segment at index {}: \"{}\"", seg.first, seg.second);
  }

  debug( "State after: next_index={}, unassembled_bytes={}, available_capacity={}",
         next_index_,
         unassembled_bytes_,
         output_.writer().available_capacity() );

  // 向output写入数据
  while ( !segments_.empty() && segments_.begin()->first == next_index_ ) {
    const auto& seg = segments_.begin()->second;
    output_.writer().push( seg );
    next_index_ += seg.size();
    unassembled_bytes_ -= seg.size();
    segments_.erase( segments_.begin() );
    debug("Wrote segment to output, next_index now {}", next_index_);
  }
  // 如果已经接收到EOF，并且所有数据都已经写入，则关闭流
  if ( eof_received_ && unassembled_bytes_ == 0 && next_index_ >= eof_index_ ) {
    debug("All data received and written, closing output.");
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  // debug( "unimplemented count_bytes_pending() called" );
  return {
    unassembled_bytes_
  };
}
