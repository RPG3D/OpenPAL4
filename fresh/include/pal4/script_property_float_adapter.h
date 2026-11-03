#pragma once

#include "pal4/script_property_forwarders.h"

#include <cstdint>

namespace pal4 {

class IScriptPropertyFloatRuntime : public IScriptPropertyForwardRuntime {
public:
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class ScriptPropertyFloatAdapter final {
public:
    // IDA 0x56eb40-0x56ebf0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IScriptPropertyFloatRuntime& runtime);
    static void ClearFlag(std::uint32_t object,
                          IScriptPropertyFloatRuntime& runtime);
    static void SetFlag(std::uint32_t object,
                        IScriptPropertyFloatRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult ForwardAtC8(
        std::uint32_t object, ScriptPropertyForwarders::Arguments arguments,
        IScriptPropertyFloatRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult ReadPlus005(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        IScriptPropertyFloatRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult ReadPlus010(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        IScriptPropertyFloatRuntime& runtime);
    [[nodiscard]] static ScriptForwardResult ReadPlus020(
        std::uint32_t object, std::uint32_t first, std::uint32_t second,
        IScriptPropertyFloatRuntime& runtime);
};

}  // namespace pal4
