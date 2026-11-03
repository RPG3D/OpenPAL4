#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ConfigCompositeState final {
    std::array<std::uint32_t, 27u> words{};
};

static_assert(sizeof(ConfigCompositeState) == 108);

class IConfigCompositeRuntime {
public:
    virtual ~IConfigCompositeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateNode(
        std::uint32_t size) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FreeNode(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyClump(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t LoadClump(
        const char* source, std::uint32_t flags,
        std::uint32_t texture_context) = 0;
    virtual void AddClumpBounds(std::uint32_t clump, std::uint32_t mode,
                                ConfigCompositeState& state,
                                std::uint32_t bounds_word_index) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomic(
        std::uint32_t clump, std::uint32_t callback,
        ConfigCompositeState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomicWithContext(
        std::uint32_t clump, std::uint32_t callback,
        const std::array<std::uint32_t, 5u>& context) = 0;
};

class ConfigCompositeRuntimeStub final : public IConfigCompositeRuntime {
public:
    [[nodiscard]] std::uint32_t AllocateNode(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t FreeNode(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t DestroyClump(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GameInstance() override { return 0u; }
    [[nodiscard]] std::uint32_t LoadClump(
        const char*, std::uint32_t, std::uint32_t) override { return 0u; }
    void AddClumpBounds(std::uint32_t, std::uint32_t,
                        ConfigCompositeState&, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ForEachAtomic(
        std::uint32_t, std::uint32_t,
        ConfigCompositeState&) override { return 0u; }
    [[nodiscard]] std::uint32_t ForEachAtomicWithContext(
        std::uint32_t, std::uint32_t,
        const std::array<std::uint32_t, 5u>&) override { return 0u; }
};

class ConfigComposite final {
public:
    static constexpr std::uint32_t kVtable = 0x84a040u;

    // IDA 0x5fedd0.
    [[nodiscard]] static ConfigCompositeState& Initialize(
        ConfigCompositeState& state, std::uint32_t native_object_address,
        IConfigCompositeRuntime& runtime);

    // IDA 0x5fee90 and 0x5feff0.
    [[nodiscard]] static std::uint32_t CleanupFull(
        ConfigCompositeState& state, IConfigCompositeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupResources(
        ConfigCompositeState& state, IConfigCompositeRuntime& runtime);

    // IDA 0x5ff0f0, 0x5ff140 and 0x5ff180.
    [[nodiscard]] static std::uint32_t LoadFloorClump(
        ConfigCompositeState& state, const char* source,
        IConfigCompositeRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadIntriClump(
        ConfigCompositeState& state, const char* source,
        IConfigCompositeRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadWallClump(
        ConfigCompositeState& state, const char* source,
        IConfigCompositeRuntime& runtime);

    // IDA 0x5ff1c0.
    [[nodiscard]] static std::uint32_t SetClumpProperties(
        std::uint32_t native_object_address, std::uint32_t clump,
        std::uint32_t mode, std::uint32_t flag1, std::uint32_t flag2,
        std::uint32_t callback, IConfigCompositeRuntime& runtime);

    // IDA 0x5ff210.
    [[nodiscard]] static std::uint32_t UnregisterClumpAtomics(
        ConfigCompositeState& state, std::uint32_t clump,
        IConfigCompositeRuntime& runtime);

    // IDA 0x5ff920; list_index selects one of four embedded list objects.
    [[nodiscard]] static std::uint32_t CleanupList(
        ConfigCompositeState& state, std::uint32_t list_index,
        IConfigCompositeRuntime& runtime);

    // IDA 0x5ff970.
    [[nodiscard]] static std::uint32_t& RemoveListNode(
        ConfigCompositeState& state, std::uint32_t list_index,
        std::uint32_t node, std::uint32_t& next_cursor,
        IConfigCompositeRuntime& runtime);

    // IDA 0x5ff9e0, 0x5ffa80, 0x5ffb20, 0x5ffbc0 and 0x5ffc60.
    [[nodiscard]] static std::uint32_t RegisterFloorAtomic(
        ConfigCompositeState& state, std::uint32_t atomic,
        IConfigCompositeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterIntriAtomic(
        ConfigCompositeState& state, std::uint32_t atomic,
        IConfigCompositeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterWallAtomic(
        ConfigCompositeState& state, std::uint32_t atomic,
        IConfigCompositeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterConfiguredAtomic(
        ConfigCompositeState& state, std::uint32_t atomic,
        const std::array<std::uint32_t, 5u>& context,
        IConfigCompositeRuntime& runtime);
    [[nodiscard]] static std::uint32_t UnregisterAtomic(
        ConfigCompositeState& state, std::uint32_t atomic,
        IConfigCompositeRuntime& runtime);
};

}  // namespace pal4
