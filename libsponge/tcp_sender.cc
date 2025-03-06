#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "util/exception.h"
#include "wrapping_integers.hh"

#include <cstdint>
#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : isn_(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , initial_retransmission_timeout_{retx_timeout}
    , stream_(capacity)
    , retransmission_timeout_{initial_retransmission_timeout_} {}

uint64_t TCPSender::bytes_in_flight() const { return bytes_in_flight_; }

void TCPSender::fill_window() {
    TCPSegment seg;
    if (!has_sender_syn_) {
        has_sender_syn_ = true;
        seg.header().syn = true;
        --peer_window_size_;
    }

    seg.header().seqno = wrap(next_seqno_, isn_);

    seg.payload() = stream_.read(std::min(peer_window_size_, TCPConfig::MAX_PAYLOAD_SIZE));
    peer_window_size_ -= seg.payload().size();
    
    if (!has_sender_fin_ and stream_.eof() and peer_window_size_ > 0) {
        --peer_window_size_;
        has_sender_fin_ = true;
        seg.header().fin = true;
    }

    if (seg.length_in_sequence_space() > 0) {
        next_seqno_ += seg.length_in_sequence_space();
        bytes_in_flight_ += seg.length_in_sequence_space();
        unack_segments_.push(seg);
        segments_out_.push(seg);
    }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    ASSERT_D(has_sender_syn_, "SYN should sent when ack received");

    peer_window_size_ = window_size;
    uint64_t ackno_abs = unwrap(ackno, isn_, next_seqno_);

    bool has_ack = false;
    while (!unack_segments_.empty()) {
        uint64_t seg_end = unwrap(unack_segments_.front().header().seqno, isn_, next_seqno_) +
                           unack_segments_.front().length_in_sequence_space();
        if (ackno_abs >= seg_end) {
            bytes_in_flight_ -= unack_segments_.front().length_in_sequence_space();
            unack_segments_.pop();
            has_ack = true;
        } else {
            break;
        }
    }

    if (has_ack) {
        consecutive_retransmission_cnt_ = 0;
        retransmission_timeout_ = initial_retransmission_timeout_;
        first_unack_time_ = 0;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    first_unack_time_ += ms_since_last_tick;
    if (first_unack_time_ >= retransmission_timeout_) {
        consecutive_retransmission_cnt_++;
        retransmission_timeout_ <<= 1;
        first_unack_time_ = 0;
        segments_out_.push(unack_segments_.front());
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return consecutive_retransmission_cnt_; }

void TCPSender::send_empty_segment() {
    ASSERT_D(has_sender_syn_, "SYN should sent when sending empty segment");
    TCPSegment seg;
    seg.header().seqno = wrap(next_seqno_, isn_);
    segments_out_.push(seg);
}
