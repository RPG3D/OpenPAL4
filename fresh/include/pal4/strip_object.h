#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct StripObjectState final {
    std::array<std::uint32_t, 75> words{};
};
static_assert(sizeof(StripObjectState) == 0x12cu);

class IStripObjectRuntime {
public:
    virtual ~IStripObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ConstructorStackByte() = 0;
    virtual void FreeBuffer(std::uint32_t address) = 0;
    virtual void FreeObject(StripObjectState& state) = 0;
};

class StripObjectRuntimeStub final : public IStripObjectRuntime {
public:
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint8_t ConstructorStackByte() override { return 0u; }
    void FreeBuffer(std::uint32_t) override {}
    void FreeObject(StripObjectState&) override {}
};

class IStripObjectPlaybackRuntime {
public:
    virtual ~IStripObjectPlaybackRuntime() = default;
    [[nodiscard]] virtual std::int32_t StartFacet(
        StripObjectState& state, std::uint32_t offset) = 0;
    [[nodiscard]] virtual double Duration(StripObjectState& state) = 0;
};

class StripObject final {
public:
    // IDA 0x617e30; factory 0x63e310 allocates 0x12c bytes.
    static StripObjectState& Construct(
        StripObjectState& state, IStripObjectRuntime& runtime);
    // IDA 0x617f60, previously unmarked vtable target.
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        StripObjectState& state, std::uint32_t flags,
        IStripObjectRuntime& runtime);
    // IDA 0x617f80.
    static void Destroy(StripObjectState& state,
                        IStripObjectRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x618a80.
    static void SetEnabled(StripObjectState& state, std::uint8_t enabled,
                           IStripObjectPlaybackRuntime& runtime);
};

}  // namespace pal4
