#include "byte_stream.hh"
#include <algorithm>
using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), buffer_(), closed_( false ), bytes_pushed_( 0 ), bytes_popped_( 0 )
{}

void Writer::push( string data )
{
  if ( is_closed() || data.empty() ) {
    return;
  }
  // The data may not be push into the buffer
  uint64_t to_write = std::min( (uint64_t)data.size(), available_capacity() );
  if ( to_write > 0 ) {
    buffer_.insert( buffer_.end(), data.begin(), data.begin() + to_write );
    bytes_pushed_ += to_write;
  } else {
    return;
  }
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  // 可用容量 = 总容量 - 当前缓冲区中的字节数
  // 当前缓冲区中的字节数 = 已推入的总字节数 - 已弹出的总字节数
  uint64_t buffered_bytes = bytes_pushed_ > bytes_popped_ ? bytes_pushed_ - bytes_popped_ : 0;
  return capacity_ > buffered_bytes ? capacity_ - buffered_bytes : 0;
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  if ( buffer_.empty() ) {
    return std::string_view();
  }
  return string_view( buffer_.data(), bytes_buffered() );
}

void Reader::pop( uint64_t len )
{
  if ( len > buffer_.size() ) {
    set_error();
    return;
  }
  buffer_.erase( buffer_.begin(), buffer_.begin() + len );
  bytes_popped_ += len;
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.empty();
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
