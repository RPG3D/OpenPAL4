#pragma once

#include <cstdint>

namespace pal4 {

class IGameArenaRecordMemory {
public:
    virtual ~IGameArenaRecordMemory() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class GameArenaRecordMemoryStub final : public IGameArenaRecordMemory {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class GameArenaRecordCompactor final {
public:
    // IDA 0x51b5b0. Moves 12-byte records [source, end) to destination.
    [[nodiscard]] static std::uint32_t Compact(
        std::uint32_t range, std::uint32_t destination,
        std::uint32_t source, IGameArenaRecordMemory& memory);
};

}  // namespace pal4
