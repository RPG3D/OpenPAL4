#pragma once

#include "pal4/script_entity_callbacks.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptNpcLifecycleRuntime : public IScriptNpcVisibilityRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateOrGetInfo(
        std::uint32_t manager, std::string_view name,
        std::string_view resource, std::array<std::uint32_t, 3> position,
        std::array<std::uint32_t, 3> rotation) = 0;
    [[nodiscard]] virtual std::uint32_t CreateAndLoadNpc(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t resource, std::uint32_t position,
        std::uint32_t rotation, std::uint32_t info,
        std::uint32_t enabled) = 0;
    virtual void DeleteNpc(std::uint32_t manager,
                           std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveNpcInfo(
        std::uint32_t manager, std::uint32_t name) = 0;
};

class ScriptNpcLifecycle final {
public:
    // IDA 0x5d3c20 and 0x5d3d20. Names are native record +4 pointers.
    [[nodiscard]] static std::uint32_t Create(
        const char* name, const char* resource,
        std::array<std::uint32_t, 3> position,
        IScriptNpcLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        const char* name, IScriptNpcLifecycleRuntime& runtime);
};

}  // namespace pal4
