#include "tcp_connection.hh"

#include <iostream>
#include <sys/socket.h>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

bool TCPConnection::real_send() {
    bool is_send = false;
    while (!sender_.segments_out().empty()) {
        is_send = true;
        auto &seg = sender_.segments_out().front();
        if (receiver_.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = receiver_.ackno().value();
            seg.header().win = receiver_.window_size();
        }
        segments_out_.push(std::move(seg));
        sender_.segments_out().pop();
    }
    return is_send;
}

void TCPConnection::self_reset() {
    is_active_ = false;
    inbound_stream().set_error();
    outbound_stream().set_error();
}

void TCPConnection::send_and_self_reset() {
    sender_.send_empty_segment();
    sender_.segments_out().back().header().rst = true;
    real_send();
    self_reset();
}

size_t TCPConnection::remaining_outbound_capacity() const { return sender_.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return sender_.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return receiver_.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return time_since_last_segment_received_; }

bool TCPConnection::check_inbound_ended() {
    return receiver_.stream_out().input_ended() and receiver_.unassembled_bytes() == 0;
}

// 四次挥手，最后一次的time_wait状态待实现
void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!is_active_) {
        return;
    }

    if (seg.header().rst) {
        self_reset();
        return;
    }

    time_since_last_segment_received_ = 0;

    receiver_.segment_received(seg);
    if (seg.header().ack) {
        sender_.ack_received(seg.header().ackno, seg.header().win);
    }

    if (!outbound_stream().eof() and check_inbound_ended()) {
        linger_after_streams_finish_ = false;
    }

    if (seg.length_in_sequence_space() > 0) {
        sender_.fill_window();
        bool is_send = real_send();
        if (!is_send) {
            sender_.send_empty_segment();
            real_send();
        }
    }
}

bool TCPConnection::active() const { return is_active_; }

size_t TCPConnection::write(const string &data) {
    size_t written = outbound_stream().write(data);
    sender_.fill_window();
    real_send();
    return written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!is_active_) {
        return;
    }

    sender_.tick(ms_since_last_tick);
    real_send();
    time_since_last_segment_received_ += ms_since_last_tick;
    if (sender_.consecutive_retransmissions() >= cfg_.MAX_RETX_ATTEMPTS) {
        send_and_self_reset();
    }

    if (outbound_stream().eof() and check_inbound_ended() and sender_.bytes_in_flight() == 0) {
        if (linger_after_streams_finish_) {
            curr_linger_time_ += ms_since_last_tick;
            if (curr_linger_time_ >= 10 * cfg_.rt_timeout) {
                is_active_ = false;
            }
        } else {
            is_active_ = false;
        }
    }
}

void TCPConnection::end_input_stream() {
    outbound_stream().end_input();
    sender_.fill_window();
    real_send();
}

void TCPConnection::connect() {
    sender_.fill_window();
    real_send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_and_self_reset();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
