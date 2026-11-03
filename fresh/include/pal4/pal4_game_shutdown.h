#pragma once

#include <cstdint>

namespace pal4 {

class IPal4GameShutdownRuntime {
public:
    virtual ~IPal4GameShutdownRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ShutdownGameObject(
        std::uint32_t object) = 0;

    [[nodiscard]] virtual std::uint32_t FontManager() = 0;
    virtual void GetFontTextures(std::uint32_t manager) = 0;
    virtual void GetFontTextureStates(std::uint32_t manager) = 0;
    virtual void GetEffectScriptSlot284() = 0;
    virtual void GetEffectScriptSlot288() = 0;
    [[nodiscard]] virtual std::uint32_t ArenaEffectManager() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupCollection(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupCollectionAlternate(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptState() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptProperty288() = 0;
};

class Pal4GameShutdown final {
public:
    static constexpr std::uint32_t kGameObjectOffset = 0x18cu;
    static constexpr std::uint32_t kCombatObjectOffset = 0x1a8u;
    static constexpr std::uint32_t kCollectionOffset = 0x104u;

    // IDA 0x5c83a0 and 0x5c83d0.
    [[nodiscard]] static std::int32_t ShutdownIfRequested(
        std::int32_t requested, IPal4GameShutdownRuntime& runtime);
    [[nodiscard]] static std::int32_t CheckAndShutdown(
        IPal4GameShutdownRuntime& runtime);

    // IDA 0x5c8410 and 0x5c8440, including their shared tail blocks.
    [[nodiscard]] static std::uint32_t CleanupResources(
        IPal4GameShutdownRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupAlternateResources(
        IPal4GameShutdownRuntime& runtime);
};

}  // namespace pal4
