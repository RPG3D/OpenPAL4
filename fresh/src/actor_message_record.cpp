#include "pal4/actor_message_record.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t ActorMessageRecord::ReadDword(
    const std::uint32_t offset) const {
    if (offset > kBytes - 4u) throw std::out_of_range("message field");
    return static_cast<std::uint32_t>(bytes_[offset]) |
           (static_cast<std::uint32_t>(bytes_[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes_[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes_[offset + 3u]) << 24u);
}

void ActorMessageRecord::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) {
    if (offset > kBytes - 4u) throw std::out_of_range("message field");
    for (std::uint32_t byte = 0u; byte < 4u; ++byte) {
        bytes_[offset + byte] =
            static_cast<std::uint8_t>(value >> (byte * 8u));
    }
}

std::uint32_t ActorMessageRecord::CopyFrom(
    const ActorMessageRecord& source) {
    bool terminated = false;
    for (std::uint32_t index = 0u; index < kTextBytes; ++index) {
        const auto value = terminated ? 0u : source.Bytes()[index];
        bytes_[index] = static_cast<std::uint8_t>(value);
        terminated = terminated || value == 0u;
    }
    const auto result = source.ReadDword(0x44u);
    WriteDword(0x44u, result);
    WriteDword(0x40u, source.ReadDword(0x40u));
    return result;
}

void ActorMessageRecord::Initialize() noexcept {
    WriteDword(0x40u, 0u);
    WriteDword(0x44u, 0u);
    bytes_[0] = 0u;
}

std::uint32_t ActorMessageRecordFactory::Create(
    IActorMessageRecordRuntime& runtime) {
    const auto address = runtime.Allocate(ActorMessageRecord::kBytes);
    if (address == 0u) return 0u;
    runtime.Access(address).Initialize();
    return address;
}

}  // namespace pal4
