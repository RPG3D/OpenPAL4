#pragma once

#include <cstdint>
#include <initializer_list>

namespace pal4 {

enum class ArenaActorCall : std::uint8_t {
    ConstructResourceManager,
    InitializeObjectState,
    CleanupObjectState,
    DestroyResourceManager,
    CleanupObjectData,
    GetGameInstance,
    CleanupClump,
    RenderClump,
    ProcessAnimation,
    UpdateAndRenderClump,
    UpdateInterpolator,
    InitializeName,
    FindScriptMarker,
    GetArenaManager,
    ProcessArenaScript,
    ProcessEmbeddedScript,
    LoadClump,
    HideUserDataAtomics,
    LoadAnimationData,
    CleanupScripts,
    CleanupName,
};

class IArenaActorRuntime {
public:
    virtual ~IArenaActorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        ArenaActorCall operation,
        std::initializer_list<std::uint32_t> arguments) = 0;
};

class ArenaActorRuntimeStub final : public IArenaActorRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Call(
        ArenaActorCall,
        std::initializer_list<std::uint32_t>) override { return 0u; }
};

class ArenaActorResource final {
public:
    static constexpr std::uint32_t kObjectStateOffset = 368u;
    static constexpr std::uint32_t kResourceVtable = 0x8448a0u;
    static constexpr std::uint32_t kObjectStateVtable = 0x8448ccu;

    // IDA 0x520cc0/0x520d40: construct and destroy the paired objects.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IArenaActorRuntime& runtime);
    static void Destroy(std::uint32_t object, IArenaActorRuntime& runtime);
    // IDA 0x520da0: clear object data and its resource clump.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t object, IArenaActorRuntime& runtime);
    // IDA 0x520dd0: render the clump then process object animation.
    [[nodiscard]] static std::uint32_t RenderAndAnimate(
        std::uint32_t object, std::uint32_t pass,
        IArenaActorRuntime& runtime);
    // IDA 0x520df0: update both clump and animation interpolator.
    [[nodiscard]] static std::uint32_t Update(
        float delta, IArenaActorRuntime& runtime);
    // IDA 0x520e10: select script source and load an animated clump.
    [[nodiscard]] static bool ExecuteScriptAndLoad(
        std::uint32_t object, std::uint32_t resource_id,
        std::uint32_t name, std::uint32_t options,
        std::uint32_t temporary_name, IArenaActorRuntime& runtime);
};

}  // namespace pal4
