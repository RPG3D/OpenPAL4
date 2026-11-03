#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
enum class MediaByteTransferKind { media, sound };

struct MediaByteTransferTicket final {
    MediaByteTransferKind kind{};
    std::uint64_t generation{}, id{};
};

class MediaByteTransferState final {
public:
    void Begin(MediaByteTransferTicket ticket, bool has_bytes) {
        if (pending_) throw std::logic_error("overlapping media byte transfer");
        if (has_bytes) pending_=ticket;
    }
    const std::optional<MediaByteTransferTicket>& Pending() const noexcept { return pending_; }
    std::optional<MediaByteTransferTicket> Complete() noexcept { return std::exchange(pending_,{}); }
    std::optional<MediaByteTransferTicket> Fail() noexcept { return std::exchange(pending_,{}); }
    void Reset() noexcept { pending_.reset(); }
private:
    std::optional<MediaByteTransferTicket> pending_;
};
}
