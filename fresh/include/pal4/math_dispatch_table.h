#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct MathDispatchTable final {
    static constexpr std::uint32_t kNativeAddress = 0x93cd74u;
    static constexpr std::uint32_t kTangentSlot = 6u;
    static constexpr std::uint32_t kArctangentSlot = 7u;
    std::array<std::uint32_t, 10u> entries{};
};

static_assert(sizeof(MathDispatchTable) == 40);

class MathDispatchInitializer final {
public:
    // IDA 0x601c90.
    static void Initialize(MathDispatchTable& table);
    // IDA 0x601d00, attached tail of 0x601d60.
    static void InstallSse(MathDispatchTable& table);
};

struct MathCpuFeatures final {
    std::uint8_t mmx = 0u;
    std::uint8_t sse = 0u;
    std::uint8_t sse2 = 0u;
    std::uint8_t three_d_now = 0u;
};

static_assert(sizeof(MathCpuFeatures) == 4);

class IMathRuntimeInitialization {
public:
    virtual ~IMathRuntimeInitialization() = default;
    virtual void CpuBrand(char* output, std::uint32_t capacity) = 0;
    virtual void CpuSerial(std::uint32_t selector, char* output,
                           std::uint32_t capacity) = 0;
    [[nodiscard]] virtual std::uint8_t HasMmx() = 0;
    [[nodiscard]] virtual std::uint8_t HasSse() = 0;
    [[nodiscard]] virtual std::uint8_t HasSse2() = 0;
    [[nodiscard]] virtual std::uint8_t Has3DNow() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeLogger() = 0;
    virtual void AppendCpuReport(std::uint32_t logger,
                                 const char* format,
                                 const char* brand,
                                 const char* serial,
                                 const MathCpuFeatures& features) = 0;
    virtual void InitializeSquareRootTables() = 0;
};

class MathRuntimeInitializationStub : public IMathRuntimeInitialization {
public:
    void CpuBrand(char*, std::uint32_t) override {}
    void CpuSerial(std::uint32_t, char*, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t HasMmx() override { return 0u; }
    [[nodiscard]] std::uint8_t HasSse() override { return 0u; }
    [[nodiscard]] std::uint8_t HasSse2() override { return 0u; }
    [[nodiscard]] std::uint8_t Has3DNow() override { return 0u; }
    [[nodiscard]] std::uint32_t InitializeLogger() override { return 0u; }
    void AppendCpuReport(std::uint32_t, const char*, const char*,
                         const char*, const MathCpuFeatures&) override {}
    void InitializeSquareRootTables() override {}
};

class MathRuntimeInitializer final {
public:
    // IDA 0x601d60.
    [[nodiscard]] static std::uint8_t Initialize(
        MathCpuFeatures& features, MathDispatchTable& table,
        IMathRuntimeInitialization& runtime);
};

}  // namespace pal4
