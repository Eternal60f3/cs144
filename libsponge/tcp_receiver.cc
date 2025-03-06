#include "tcp_receiver.hh"

#include "util/exception.h"
#include "wrapping_integers.hh"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <optional>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn) {
        peer_isn_ = seg.header().seqno;
    }
    if (peer_isn_ == std::nullopt) {
        return;
    }
    if (seg.header().fin) {
        has_received_fin_ = true;
    }

    ASSERT_D(peer_isn_.has_value(), "peer_isn_ should have value");
    size_t index = unwrap(seg.header().seqno, peer_isn_.value(), reassembler_.first_unassembled_index() + 1);
    reassembler_.push_substring(seg.payload().copy(), index - !seg.header().syn, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (peer_isn_.has_value()) {
        return std::make_optional(wrap(reassembler_.first_unassembled_index() + peer_isn_.has_value() +
                                           (has_received_fin_ and reassembler_.unassembled_bytes() == 0),
                                       peer_isn_.value()));
    } else {
        return std::nullopt;
    }
}

size_t TCPReceiver::window_size() const { return reassembler_.remaining_capacity(); }
