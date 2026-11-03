#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct DynamicStripObjectState final {
    std::array<std::uint32_t, 57u> words{};
};

static_assert(sizeof(DynamicStripObjectState) == 228u);

class IDynamicStripObjectRuntime {
public:
    virtual ~IDynamicStripObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CleanupResources(DynamicStripObjectState& state) = 0;
    virtual void ReleaseRange(std::uint32_t begin, std::uint32_t end) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CleanupConfig(DynamicStripObjectState& state) = 0;
    virtual void FreeObject(DynamicStripObjectState& state) = 0;
};

class DynamicStripObjectRuntimeStub final : public IDynamicStripObjectRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void CleanupResources(DynamicStripObjectState&) override {}
    void ReleaseRange(std::uint32_t, std::uint32_t) override {}
    void Free(std::uint32_t) override {}
    void CleanupConfig(DynamicStripObjectState&) override {}
    void FreeObject(DynamicStripObjectState&) override {}
};

class DynamicStripObject final {
public:
    // IDA 0x60e100. Both stack-derived bytes are explicit inputs.
    static DynamicStripObjectState& Construct(
        DynamicStripObjectState& state, std::uint8_t first_byte,
        std::uint8_t second_byte, IDynamicStripObjectRuntime& runtime);
    // IDA 0x60e230, life stored at object +112 by visual setup.
    [[nodiscard]] static float GetLife(
        const DynamicStripObjectState& state);
    // IDA 0x60eb00/0x60eb10.
    [[nodiscard]] static std::uint32_t ResourceHandle(
        const DynamicStripObjectState& state);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x60faa0; updates the mode byte consumed by strip update.
    [[nodiscard]] static std::uint8_t SetModeByte(
        DynamicStripObjectState& state, std::uint8_t mode);
    // IDA 0x60e260 and deleting-destructor code island 0x60e240.
    static void Destroy(DynamicStripObjectState& state,
                        IDynamicStripObjectRuntime& runtime);
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        DynamicStripObjectState& state, std::uint32_t flags,
        IDynamicStripObjectRuntime& runtime);
};

}  // namespace pal4
