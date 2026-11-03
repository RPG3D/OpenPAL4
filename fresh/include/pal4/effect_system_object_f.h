#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IEffectSystemObjectFConfigRuntime;

struct EffectSystemObjectFState final {
    std::array<std::uint32_t, 42u> words{};
};

static_assert(sizeof(EffectSystemObjectFState) == 0xa8);

class IEffectSystemObjectFRuntime {
public:
    virtual ~IEffectSystemObjectFRuntime() = default;
    virtual void InitializeXmlDocument(
        EffectSystemObjectFState& state, std::uint32_t byte_offset) = 0;
    virtual void DestroyXmlDocument(
        EffectSystemObjectFState& state, std::uint32_t byte_offset) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EffectSystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class EffectSystemObjectFRuntimeStub final
    : public IEffectSystemObjectFRuntime {
public:
    void InitializeXmlDocument(EffectSystemObjectFState&,
                               std::uint32_t) override {}
    void DestroyXmlDocument(EffectSystemObjectFState&,
                            std::uint32_t) override {}
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t EffectSystemInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class IEffectSystemObjectFPlaybackRuntime {
public:
    virtual ~IEffectSystemObjectFPlaybackRuntime() = default;
    virtual void QueryTaskValues(EffectSystemObjectFState& state,
                                 std::array<std::int32_t, 4>& values) = 0;
    [[nodiscard]] virtual double Duration(
        EffectSystemObjectFState& state) = 0;
};

class EffectSystemObjectF final {
public:
    // IDA 0x604dd0. Native stack byte at +152 is indeterminate.
    static EffectSystemObjectFState& Construct(
        EffectSystemObjectFState& state, std::uint8_t initial_byte,
        IEffectSystemObjectFRuntime& runtime);
    // IDA 0x604f00.
    static void Destroy(EffectSystemObjectFState& state,
                        IEffectSystemObjectFRuntime& runtime);
    // IDA 0x6057b0.
    [[nodiscard]] static std::uint32_t ObjectHandle(
        const EffectSystemObjectFState& state,
        IEffectSystemObjectFRuntime& runtime);
    // IDA 0x6057d0.
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x6057e0. Only the low input byte is stored at object +11.
    static void SetTimedMode(EffectSystemObjectFState& state,
                             std::uint32_t enabled,
                             IEffectSystemObjectFPlaybackRuntime& runtime);
    // IDA 0x605850.
    [[nodiscard]] static std::uint8_t LoadConfiguration(
        EffectSystemObjectFState& state, std::uint32_t document,
        IEffectSystemObjectFConfigRuntime& runtime);
    // IDA 0x605d90.
    [[nodiscard]] static std::uint8_t ReapplyCachedConfiguration(
        EffectSystemObjectFState& state,
        IEffectSystemObjectFConfigRuntime& runtime);
};

}  // namespace pal4
