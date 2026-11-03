#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct RtkPrtStdAtomicObjectState final {
    std::array<std::uint32_t, 34u> words{};
};

static_assert(sizeof(RtkPrtStdAtomicObjectState) == 136u);

class IRtkPrtStdAtomicObjectRuntime {
public:
    virtual ~IRtkPrtStdAtomicObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
};

class RtkPrtStdAtomicObjectRuntimeStub final
    : public IRtkPrtStdAtomicObjectRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
};

class IRtkPrtStdAtomicUpdateRuntime {
public:
    virtual ~IRtkPrtStdAtomicUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t UpdateState(
        std::uint32_t resource, double elapsed,
        std::uint32_t argument, std::uint8_t mode) = 0;
};

class RtkPrtStdAtomicUpdateRuntimeStub final
    : public IRtkPrtStdAtomicUpdateRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t UpdateState(
        std::uint32_t, double, std::uint32_t, std::uint8_t) override { return 0u; }
};

class RtkPrtStdAtomicObject final {
public:
    // IDA 0x611530; factory 0x63e490 allocates 0x88 bytes.
    static RtkPrtStdAtomicObjectState& Construct(
        RtkPrtStdAtomicObjectState& state,
        IRtkPrtStdAtomicObjectRuntime& runtime);
    // IDA 0x611620, via the derived vtable slot at 0x84a80c.
    [[nodiscard]] static double GetDuration(
        const RtkPrtStdAtomicObjectState& state,
        IRtkPrtStdAtomicObjectRuntime& runtime);
    // IDA 0x611650.
    static void Destroy(RtkPrtStdAtomicObjectState& state,
                        IRtkPrtStdAtomicObjectRuntime& runtime);
    // IDA 0x611740.
    [[nodiscard]] static std::uint8_t Update(
        const RtkPrtStdAtomicObjectState& state,
        IRtkPrtStdAtomicUpdateRuntime& runtime);
    // IDA 0x612980.
    [[nodiscard]] static std::uint32_t ResourceProperty(
        const RtkPrtStdAtomicObjectState& state,
        IRtkPrtStdAtomicObjectRuntime& runtime);
    // IDA 0x6129a0.
    [[nodiscard]] static std::string_view ClassName();
};

}  // namespace pal4
