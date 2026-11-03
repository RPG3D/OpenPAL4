#pragma once

#include <cstdint>

namespace pal4 {

class IScriptRoundTimesRuntime {
public:
    virtual ~IScriptRoundTimesRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SaveSettings(std::uint32_t global_data) = 0;
};

class ScriptRoundTimes final {
public:
    // IDA 0x5d85f0, giAddRoundTimes.
    static void Increment(IScriptRoundTimesRuntime& runtime);
};

}  // namespace pal4
