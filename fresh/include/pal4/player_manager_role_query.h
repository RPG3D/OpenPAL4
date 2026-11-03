#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerManagerRoleQueryRuntime {
public:
    virtual ~IPlayerManagerRoleQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Manager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
};

class PlayerManagerRoleQueryRuntimeStub : public IPlayerManagerRoleQueryRuntime {
public:
    std::uint32_t Manager() override;
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
};

class PlayerManagerRoleQuery final {
public:
    static constexpr std::uint32_t kNamePointerOffset = 0xa1cu;
    static constexpr std::uint32_t kNameLengthOffset = 0xa20u;

    // IDA 0x441a90: checks the first player's name, not its membership flag.
    [[nodiscard]] static bool FirstPlayerIs102(
        IPlayerManagerRoleQueryRuntime& runtime);
};

}  // namespace pal4
