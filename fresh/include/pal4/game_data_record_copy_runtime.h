#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataRecordCopyRuntime {
public:
    virtual ~IGameDataRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t count) = 0;
    virtual void ResetText(std::uint32_t destination) = 0;
    [[nodiscard]] virtual std::uint32_t AssignText(
        std::uint32_t destination, std::uint32_t source) = 0;
};

}  // namespace pal4
