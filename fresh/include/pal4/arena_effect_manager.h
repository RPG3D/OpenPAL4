#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>

namespace pal4 {

enum class ArenaEffectCall : std::uint8_t {
    InitializeBase,
    ResetExisting,
    RemoveUncountedTreeNodes,
    ResetFirstTreeElement,
    DestroyBase,
    InitializeGameManagers,
    GetEffectSingleton,
    StopActive,
    CleanupEffects,
    Disable,
    RenderPasses,
    UpdateRenderParameters,
    HandleScriptExecution,
    GetGameArenaManager,
    AddClump,
    Enable,
    Draw,
    RenderTransform,
};

struct ArenaEffectSharedState {
    std::uint32_t sentinel = 0u;
    std::uint32_t instance_count = 0u;
    std::uint32_t script_callback = 0u;
};

class IArenaEffectRuntime {
public:
    virtual ~IArenaEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        ArenaEffectCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class ArenaEffectRuntimeStub final : public IArenaEffectRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::string_view, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(
        ArenaEffectCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class ArenaEffectManager final {
public:
    static constexpr std::uint32_t kObjectSize = 0x130u;
    static constexpr std::uint32_t kVtable = 0x8448d8u;
    static constexpr std::uint32_t kSingletonId = 1049u;
    static constexpr std::string_view kSingletonName = "palGameArenaEffMgr";

    // IDA 0x5226f0/0x522730: guarded singleton registration and factory.
    [[nodiscard]] static std::uint32_t Register(
        std::uint8_t& guard, std::uint32_t& registered,
        IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        ArenaEffectSharedState& shared, IArenaEffectRuntime& runtime);
    // IDA 0x522790/0x522890: shared tree sentinel and effect lifecycle.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, ArenaEffectSharedState& shared,
        IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t object, ArenaEffectSharedState& shared,
        IArenaEffectRuntime& runtime);
    // IDA 0x522b50/0x522bc0: release nodes and the owning tree header.
    [[nodiscard]] static std::uint32_t CleanupTree(
        std::uint32_t node, ArenaEffectSharedState& shared,
        IArenaEffectRuntime& runtime);
    static void CleanupTreeContainer(
        std::uint32_t tree, ArenaEffectSharedState& shared,
        IArenaEffectRuntime& runtime);
    // IDA 0x5229b0-0x522b20: effect, script and rendering callbacks.
    [[nodiscard]] static bool InitializeGameManagers(
        std::uint32_t object, IArenaEffectRuntime& runtime);
    static void CleanupAndShutdown(
        std::uint32_t object, ArenaEffectSharedState& shared,
        IArenaEffectRuntime& runtime);
    static void CleanupAndShutdownEffects(
        std::uint32_t object, ArenaEffectSharedState& shared,
        IArenaEffectRuntime& runtime);
    static void RenderPasses(std::uint32_t object, std::uint32_t pass,
                             IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateRenderParams(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleScriptCallback(
        std::uint32_t object, std::uint32_t argument,
        ArenaEffectSharedState& shared, IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupScriptCallback(
        std::uint32_t object, ArenaEffectSharedState& shared,
        IArenaEffectRuntime& runtime);
    [[nodiscard]] static bool AddActiveObject(
        std::uint32_t clump, IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enable(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Draw(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        std::uint32_t third, std::uint32_t fourth, std::uint32_t fifth,
        std::uint32_t sixth, IArenaEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t RenderTransform(
        double value, IArenaEffectRuntime& runtime);
};

}  // namespace pal4
