#pragma once

#include "pal4/script_effect_callbacks.h"

#include <cstdint>

namespace pal4 {

class IScriptObjectEffectRuntime {
public:
    virtual ~IScriptObjectEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint8_t NeedsUpdate(
        std::uint32_t handle_address) = 0;
    virtual void UpdateContent(std::uint32_t handle_address) = 0;
    [[nodiscard]] virtual std::uint8_t IsReady(
        std::uint32_t handle_address) = 0;
    [[nodiscard]] virtual std::uint8_t DisplayAndCleanup(
        std::uint32_t handle_address) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    [[nodiscard]] virtual ScriptEffectHandle LoadFromXml(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t object_type) = 0;
    [[nodiscard]] virtual ScriptEffectHandle CreateAtAddress(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t argument,
        std::uint32_t flag) = 0;
    virtual void WriteHandle(std::uint32_t address,
                             const ScriptEffectHandle& handle) = 0;
    [[nodiscard]] virtual std::uint8_t ResetHandle(
        ScriptEffectHandle& handle) = 0;
};

class ScriptObjectEffect final {
public:
    // IDA 0x5d5c30, giEffectPlayWithOBJ.
    [[nodiscard]] static std::uint8_t Play(
        std::uint32_t effect_name_payload, std::uint32_t argument,
        std::uint32_t object_name_payload,
        IScriptObjectEffectRuntime& runtime);
    // IDA 0x5d5d20, giEffectStopWithOBJ.
    [[nodiscard]] static std::uint8_t UpdateAndShow(
        std::uint32_t object_name_payload,
        IScriptObjectEffectRuntime& runtime);
};

}  // namespace pal4
