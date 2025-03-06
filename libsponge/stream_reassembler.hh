#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <deque>
#include <string>

// \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
// possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    std::deque<char> container_;         //!< The queue to store the bytes.
    std::deque<bool> mask_;              //!< Indicates whether the current absolute index has data
    ByteStream output_;                  //!< The reassembled in-order byte stream
    size_t capacity_;                    //!< The maximum number of bytes
    size_t first_unassembled_idx_{0};    //!< The index of the first byte that has not been reassembled
    size_t total_unassembled_bytes_{0};  //!< The total number of bytes that have not been reassembled
    bool has_eof_{false};                //!< Indicates whether the last byte of the entire stream has been received

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return output_; }
    ByteStream &stream_out() { return output_; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief The index of the first byte that has not been reassembled
    size_t first_unassembled_index() const { return first_unassembled_idx_; }

    //! \brief The number of bytes that can be accepted by the reassembler
    size_t remaining_capacity() const { return capacity_ - output_.buffer_size(); }

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
