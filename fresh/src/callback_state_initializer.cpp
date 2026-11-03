#include "pal4/callback_state_initializer.h"

#include <cstdint>

namespace pal4 {
namespace {

}  // namespace

CallbackState::Storage CallbackState::Initialize(Storage storage) noexcept {
    WriteDword(storage, 0x00, 0);
    WriteDword(storage, 0x04, 0);
    WriteDword(storage, 0x10, 0);
    WriteDword(storage, 0x14, 0);
    storage[0x18] = std::byte{0};
    storage[0x19] = std::byte{1};
    storage[0x1a] = std::byte{1};
    WriteDword(storage, 0x1c, 0);
    return storage;
}

std::uint32_t CallbackState::SetDword0C(Storage& storage,
                                        const std::uint32_t value) noexcept {
    WriteDword(storage, 0x0c, value);
    return value;
}

std::uint8_t CallbackState::SetByte1A(Storage& storage,
                                     const std::uint8_t value) noexcept {
    storage[0x1a] = static_cast<std::byte>(value);
    return value;
}

std::uint32_t CallbackState::ReadDword(const Storage& storage,
                                      const std::size_t offset) noexcept {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        value |= std::to_integer<std::uint32_t>(storage[offset + index])
                 << (index * 8U);
    }
    return value;
}

void CallbackState::WriteDword(Storage& storage, const std::size_t offset,
                               const std::uint32_t value) noexcept {
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        storage[offset + index] = static_cast<std::byte>(
            (value >> (index * 8U)) & 0xffU);
    }
}

bool CallbackState::IsPropertyEmpty(const Storage& storage) noexcept {
    return ReadDword(storage, 0x10) == 0;
}

}  // namespace pal4
