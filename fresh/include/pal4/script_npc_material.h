#pragma once

#include <cstdint>

namespace pal4 {

class IScriptNpcMaterialRuntime {
public:
    virtual ~IScriptNpcMaterialRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t SetMaterial(
        std::uint32_t npc, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyMaterial(
        std::uint32_t npc) = 0;
};

class ScriptNpcMaterial final {
public:
    // IDA 0x5d7830/0x5d7870.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t npc_name_object,
        std::uint32_t material_name_object,
        IScriptNpcMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t Apply(
        std::uint32_t npc_name_object,
        IScriptNpcMaterialRuntime& runtime);
};

}  // namespace pal4
