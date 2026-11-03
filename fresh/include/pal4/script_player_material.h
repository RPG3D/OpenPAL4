#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {
// Script5D7720 borrows the indexed Player and its existing material owner.
class IScriptPlayerTextureRuntime {
public:
    virtual ~IScriptPlayerTextureRuntime() = default;
    virtual std::uint32_t SetFace(std::int32_t player, std::string_view texture) = 0;
    virtual std::uint32_t ApplyFace(std::int32_t player) = 0;
    virtual std::uint32_t SetLingshaLegs(std::string_view texture) = 0;
    virtual std::uint32_t ApplyLingshaLegs() = 0;
};
}

namespace pal4 {

class IScriptPlayerMaterialRuntime {
public:
    virtual ~IScriptPlayerMaterialRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t SetMaterial(
        std::uint32_t player, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyMaterial(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t SetLegMaterial(
        std::uint32_t player, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t ApplyLegMaterial(
        std::uint32_t player) = 0;
};

class ScriptPlayerMaterial final {
public:
    // IDA 0x5d7720/0x5d7750.
    [[nodiscard]] static std::uint32_t Set(
        std::int32_t player_index, std::uint32_t name_object,
        IScriptPlayerMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t Apply(
        std::int32_t player_index, IScriptPlayerMaterialRuntime& runtime);
    // IDA 0x5d7770/0x5d77b0.
    [[nodiscard]] static std::uint32_t SetCurrent(
        std::uint32_t name_object, IScriptPlayerMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyCurrent(
        IScriptPlayerMaterialRuntime& runtime);
    // IDA 0x5d77e0/0x5d7810, giLINGSHALegsInjured/Healing.
    [[nodiscard]] static std::uint32_t SetLingshaLegs(
        std::uint32_t name_object, IScriptPlayerMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyLingshaLegs(
        IScriptPlayerMaterialRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t CurrentPlayer(
        IScriptPlayerMaterialRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResolveName(
        std::uint32_t name_object, IScriptPlayerMaterialRuntime& runtime);
};

}  // namespace pal4
