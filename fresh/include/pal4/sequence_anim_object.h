#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct SequenceAnimObjectState final {
    std::array<std::uint32_t, 42> words{};
};
static_assert(sizeof(SequenceAnimObjectState) == 0xa8u);

class ISequenceAnimObjectRuntime {
public:
    virtual ~ISequenceAnimObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class SequenceAnimObjectRuntimeStub final : public ISequenceAnimObjectRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class SequenceAnimObject final {
public:
    // IDA 0x6150b0, factory 0x63e500 allocates 0xa8 bytes.
    static SequenceAnimObjectState& Construct(
        SequenceAnimObjectState& state,
        ISequenceAnimObjectRuntime& runtime);
    // IDA 0x6151a0.
    static void Destroy(SequenceAnimObjectState& state,
                        ISequenceAnimObjectRuntime& runtime);
    // IDA 0x6159e0 / 0x615a00.
    [[nodiscard]] static std::uint32_t ResourceProperty(
        const SequenceAnimObjectState& state,
        ISequenceAnimObjectRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
};

}  // namespace pal4
