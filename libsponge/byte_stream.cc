#include "byte_stream.hh"

#include <cassert>
#include <cstddef>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

ByteStream::ByteStream(const size_t capacity) : capacity_(capacity) {}

size_t ByteStream::write(const std::string &data) {
    size_t single_byte_written = std::min(data.size(), capacity_ - container_.size());
    total_bytes_written_ += single_byte_written;
    container_.insert(container_.end(), data.begin(), data.begin() + single_byte_written);
    return single_byte_written;
}

//! \param[in] len bytes will be copied from the output side of the buffer
std::string ByteStream::peek_output(const size_t len) const {
    assert(container_.size() <= capacity_);
    if (len > container_.size()) {
        return std::string(container_.begin(), container_.end());
    } else {
        return std::string(container_.begin(), container_.begin() + len);
    }
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    assert(container_.size() <= capacity_);
    size_t single_byte_pop = std::min(len, container_.size());
    total_bytes_pop_ += single_byte_pop;
    container_.erase(container_.begin(), container_.begin() + single_byte_pop);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string res = peek_output(len);
    pop_output(len);
    return res;
}

void ByteStream::end_input() { end_input_flag_ = true; }

bool ByteStream::input_ended() const { return end_input_flag_; }

size_t ByteStream::buffer_size() const {
    assert(container_.size() <= capacity_);
    return container_.size();
}

bool ByteStream::buffer_empty() const {
    assert(container_.size() <= capacity_);
    return container_.empty();
}

bool ByteStream::eof() const { return end_input_flag_ and buffer_empty(); }

size_t ByteStream::bytes_written() const { return total_bytes_written_; }

size_t ByteStream::bytes_read() const { return total_bytes_pop_; }

size_t ByteStream::remaining_capacity() const {
    assert(container_.size() <= capacity_);
    return capacity_ - container_.size();
}
