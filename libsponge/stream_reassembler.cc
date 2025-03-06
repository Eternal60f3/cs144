#include "stream_reassembler.hh"

#include <deque>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : container_(capacity, 0)
    , mask_(capacity, false)
    , output_(capacity)
    , capacity_(capacity) {}

// \brief This function accepts a substring (aka a segment) of bytes,
// possibly out-of-order, from the logical stream, and assembles any newly
// contiguous substrings and writes them into the output stream in order.
//! \WARNING if passed_data.size() > self_capacity_, the extra bytes will be discarded
//! \WARNING 同一个index的数据只会保留第一次出现的数据
//! \detail index是指数据在segment中的绝对位置
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        has_eof_ = true;
    }

    // \brief 将data直接放入self_container_中
    // @{
    // 需要去掉与output中的内容重叠的部分
    size_t data_index = std::max(index, first_unassembled_idx_);
    size_t data_offset = std::max(0, static_cast<int>(first_unassembled_idx_) - static_cast<int>(index));
    size_t container_offset = data_index - first_unassembled_idx_;

    for (; data_offset < data.size(); ++data_offset, ++container_offset) {
        if (container_offset >= capacity_) {
            break;
        }
        if (mask_[container_offset]) {
            continue;
        }

        container_[container_offset] = data[data_offset];
        mask_[container_offset] = true;
        ++total_unassembled_bytes_;
    }
    // @}

    // 将已经有序的数据放入output_中
    // @{
    std::string tmp;
    size_t i = 0;
    // 需要判断output是否还能容纳，否则把reassembler也当作cache的一部分，用来保存数据
    while (mask_.front() and i < output_.remaining_capacity()) {
        tmp += container_.front();
        container_.pop_front();
        container_.push_back(0);
        mask_.pop_front();
        mask_.push_back(false);
        ++i;
    }
    output_.write(tmp);
    first_unassembled_idx_ += i;
    total_unassembled_bytes_ -= i;
    // @}

    // \brief 表示输入结束了
    // @{
    if (has_eof_ and total_unassembled_bytes_ == 0) {
        output_.end_input();
    }
    // @}
}

size_t StreamReassembler::unassembled_bytes() const { return total_unassembled_bytes_; }

bool StreamReassembler::empty() const { return output_.buffer_empty(); }
