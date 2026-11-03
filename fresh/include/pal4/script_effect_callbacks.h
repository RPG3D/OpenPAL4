#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ScriptEffectHandle {
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
};

class IScriptEffectRuntime {
public:
    virtual ~IScriptEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual std::uint32_t HandleScriptCallback(
        std::uint32_t manager, std::int32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupScriptCallback(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual ScriptEffectHandle CreateAtCoordinates(
        std::uint32_t manager, std::uint32_t name,
        const std::array<float, 3>& position, std::uint32_t argument,
        std::uint32_t flag) = 0;
    [[nodiscard]] virtual ScriptEffectHandle CreateAtAddress(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t argument,
        std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t ResetHandle(
        ScriptEffectHandle& handle) = 0;
};

class ScriptEffectCallbacks final {
public:
    // IDA 0x5d5a80, giCGEffPlay.
    [[nodiscard]] static std::uint32_t PlayCallback(
        std::int32_t key, IScriptEffectRuntime& runtime);
    // IDA 0x5d5aa0, giCGEffStop.
    [[nodiscard]] static std::uint32_t StopCallback(
        IScriptEffectRuntime& runtime);
    // IDA 0x5d5ab0, giEffectPlay.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t name_payload, std::uint32_t argument,
        float x, float y, float z, IScriptEffectRuntime& runtime);
    // IDA 0x5d5b10, giEffectPlayWithPlayer.
    [[nodiscard]] static std::uint32_t CreateOnPlayer(
        std::uint32_t name_payload, std::uint32_t argument,
        std::int32_t player_index, IScriptEffectRuntime& runtime);
    // IDA 0x5d5b70, giEffectPlayWithCurrentPlayer.
    [[nodiscard]] static std::uint32_t CreateOnCurrentPlayer(
        std::uint32_t name_payload, std::uint32_t argument,
        IScriptEffectRuntime& runtime);
    // IDA 0x5d5bd0, giEffectPlayWithNPC.
    [[nodiscard]] static std::uint32_t CreateOnNpc(
        std::uint32_t name_payload, std::uint32_t argument,
        std::uint32_t npc_name_payload, IScriptEffectRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t CreateOnObject(
        std::uint32_t object, std::uint32_t name_payload,
        std::uint32_t argument, IScriptEffectRuntime& runtime);
};

}  // namespace pal4
