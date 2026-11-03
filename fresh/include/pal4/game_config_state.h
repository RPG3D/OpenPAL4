#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IGameConfigMemory {
public:
    virtual ~IGameConfigMemory() = default;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class GameConfigMemoryStub final : public IGameConfigMemory {
public:
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0; }
};

class GameConfigState final {
public:
    // IDA 0x665a60.
    [[nodiscard]] static std::uint32_t FreeAndNull(std::uint32_t& slot,
                                                    IGameConfigMemory& memory);
    // IDA 0x665a80 and 0x665ac0.
    [[nodiscard]] GameConfigState& Initialize();
    [[nodiscard]] std::uint32_t Cleanup(IGameConfigMemory& memory);

    std::array<std::uint32_t, 16> words{};
};

}  // namespace pal4
