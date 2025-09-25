#pragma once

#include "byte_stream.hh"
#include <map>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) :
    output_( std::move( output ) ),
    segments_(),
    next_index_( 0 ),
    unassembled_bytes_( 0 ),
    eof_received_( false ),
    eof_index_( 0 ){}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

private:
  ByteStream output_;
  // segments_ the map that stores the segments that have been received but not yet assembled
  // key: the starting index of the segment
  // value: the data of the segment
  std::map<uint64_t, std::string> segments_;
  // next_index_ is the next index that we expect to write to the output
  // 缓冲区的first index 就是 next_index_
  uint64_t next_index_ = 0;
  // unassembled_bytes_ is the total number of bytes that have been received but not yet assembled
  size_t unassembled_bytes_ = 0;
  // eof_received_ indicates whether we have received the end of file marker
  bool eof_received_ = false;
  // eof_index_ is the index of the end of file marker
  uint64_t eof_index_ = 0;
};
