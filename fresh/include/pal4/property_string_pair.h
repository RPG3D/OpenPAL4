#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IPropertyStringPairRuntime {
public:
    virtual ~IPropertyStringPairRuntime() = default;
    [[nodiscard]] virtual int CompareBytes(std::uint32_t right,
                                           std::uint32_t left,
                                           std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class PropertyStringPair final {
public:
    using Storage = std::array<std::uint32_t, 8>;
    static constexpr std::uint32_t kEmptyStringAddress = 0x841964u;

    [[nodiscard]] static bool Compare(const Storage& left,
                                      const Storage& right,
                                      IPropertyStringPairRuntime& runtime);
    [[nodiscard]] static std::uint32_t ItemProperty(
        const Storage& object) noexcept;
    [[nodiscard]] static std::uint32_t Cleanup(
        Storage& object, IPropertyStringPairRuntime& runtime);
};

}  // namespace pal4
