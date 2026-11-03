#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPalTestResultRuntime {
public:
    virtual ~IScriptPalTestResultRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class ScriptPalTestResult final {
public:
    // IDA 0x5d86e0, giGetPalTestResult.
    [[nodiscard]] static std::uint32_t Get(
        std::int32_t index, IScriptPalTestResultRuntime& runtime);
};

}  // namespace pal4
