#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptEntityTransformRuntime {
public:
    virtual ~IScriptEntityTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindPlayer(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void SetPosition(std::uint32_t entity,
                             const std::array<float, 3>& position,
                             std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t SetRotation(
        std::uint32_t entity, const std::array<float, 3>& rotation,
        std::uint32_t flags) = 0;
};

class ScriptEntityTransform final {
public:
    // IDA 0x5d7d90, giPlayerSetPosRot(int, ...).
    [[nodiscard]] static std::uint32_t PlayerByIndex(
        std::int32_t index, float x, float y, float z, float yaw,
        IScriptEntityTransformRuntime& runtime);
    // IDA 0x5d7e00, giPlayerSetPosRot(string&, ...); uses NPC manager.
    [[nodiscard]] static std::uint32_t NpcByName(
        const char* name, float x, float y, float z, float yaw,
        IScriptEntityTransformRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Apply(
        std::uint32_t entity, float x, float y, float z, float yaw,
        IScriptEntityTransformRuntime& runtime);
};

}  // namespace pal4
