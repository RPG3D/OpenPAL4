#pragma once

#include <cstdint>

namespace pal4 {

class ICompactStringRuntime {
public:
    virtual ~ICompactStringRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t bytes) = 0;
};

class CompactStringRuntimeStub final : public ICompactStringRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t bytes) override;
};

class CompactStringBuffer final {
public:
    static constexpr std::uint32_t kEmptyBlock = 0x90f96cu;

    // IDA 0x5f0200, 0x5f0270, 0x5f0310, 0x5f0360.
    [[nodiscard]] static std::uint32_t Reallocate(
        std::uint32_t object, std::uint32_t capacity,
        ICompactStringRuntime& runtime);
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t object, std::uint32_t source, std::uint32_t length,
        ICompactStringRuntime& runtime);
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t object, std::uint32_t source, std::uint32_t length,
        ICompactStringRuntime& runtime);
    static void Initialize(std::uint32_t object, std::uint32_t length,
                           std::uint32_t capacity,
                           ICompactStringRuntime& runtime);
    // IDA 0x5f1270.
    [[nodiscard]] static std::uint32_t FindChar(
        std::uint32_t object, std::uint8_t character,
        std::uint32_t start, ICompactStringRuntime& runtime);
};

}  // namespace pal4
