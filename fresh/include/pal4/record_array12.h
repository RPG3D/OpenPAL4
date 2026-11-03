#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IRecordArray12Runtime {
public:
    virtual ~IRecordArray12Runtime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
};

class RecordArray12RuntimeStub final : public IRecordArray12Runtime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
};

class RecordArray12 final {
public:
    static constexpr std::uint32_t kRecordSize = 12u;

    // IDA 0x522380: insert count copies of a three-word value at position.
    [[nodiscard]] static bool Insert(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count, std::uint32_t value,
        IRecordArray12Runtime& runtime);
    [[nodiscard]] static bool InsertValue(
        std::uint32_t array, std::uint32_t position,
        std::uint32_t count,
        const std::array<std::uint32_t, 3>& value,
        IRecordArray12Runtime& runtime);
};

}  // namespace pal4
