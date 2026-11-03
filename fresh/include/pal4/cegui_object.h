#pragma once

#include <array>
#include <cstdint>

#include "pal4/cegui_lifecycle.h"

namespace pal4 {

class CeguiObjectState final {
public:
    static constexpr std::uint32_t kObservedBytes = 0x10u;

    [[nodiscard]] std::array<std::uint8_t, kObservedBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kObservedBytes>& Bytes()
        const noexcept { return bytes_; }

    void WriteDword(std::uint32_t offset, std::uint32_t value) noexcept {
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            bytes_[offset + index] =
                static_cast<std::uint8_t>(value >> (8u * index));
        }
    }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const noexcept {
        std::uint32_t value = 0u;
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            value |= static_cast<std::uint32_t>(bytes_[offset + index])
                     << (8u * index);
        }
        return value;
    }

private:
    std::array<std::uint8_t, kObservedBytes> bytes_{};
};

static_assert(sizeof(CeguiObjectState) == CeguiObjectState::kObservedBytes);

class ICeguiObjectRuntime {
public:
    virtual ~ICeguiObjectRuntime() = default;
    virtual void InitializeList(std::uint32_t address) = 0;
};

class ICeguiObjectListRuntime {
public:
    virtual ~ICeguiObjectListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
};

class CeguiObject final {
public:
    [[nodiscard]] static std::uint32_t Construct(
        CeguiObjectState& state, std::uint32_t address,
        std::uint8_t first_source_byte,
        std::uint8_t second_source_byte,
        std::uint8_t flag,
        ICeguiObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeList(
        CeguiObjectState& state, CeguiTreeGlobals& globals,
        ICeguiObjectListRuntime& runtime);
};

}  // namespace pal4
