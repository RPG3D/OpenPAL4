#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

class IActorManagerNamePropertyRuntime {
public:
    virtual ~IActorManagerNamePropertyRuntime() = default;
    [[noreturn]] virtual void ThrowRange() = 0;
    [[nodiscard]] virtual std::uint8_t DetachAndReset(
        StringBufferState& text) = 0;
    [[nodiscard]] virtual bool AllocateText(StringBufferState& text,
                                             std::uint32_t length,
                                             std::uint8_t mode) = 0;
    virtual void SetTextSize(StringBufferState& text,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Copy(std::uint32_t destination,
                      std::uint32_t source, std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerNameProperty final {
public:
    static constexpr std::uint32_t kOffset = 812u;
    static constexpr std::uint32_t kEmptyText = 0x841964u;

    [[nodiscard]] static std::uint32_t Address(std::uint32_t actor);
    [[nodiscard]] static std::uint8_t Assign(
        StringBufferState& destination, const StringBufferState& source,
        IActorManagerNamePropertyRuntime& runtime);
};

}  // namespace pal4
