#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptGobResetRuntime {
public:
    virtual ~IScriptGobResetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeReset(
        std::uint32_t object) = 0;
};

class ScriptGobReset final {
public:
    // IDA 0x5d8150, giGOBReset; dispatches virtual slot +0x74.
    [[nodiscard]] static std::uint32_t ByName(
        const char* name, IScriptGobResetRuntime& runtime);
};

}  // namespace pal4
