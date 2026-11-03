#pragma once

#include <cstdint>

namespace pal4 {

class IScriptObjectFactoryRuntime {
public:
    virtual ~IScriptObjectFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class ScriptObjectFactory final {
public:
    // IDA 0x56dd00.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t output, std::int32_t group,
        std::int32_t variant, IScriptObjectFactoryRuntime& runtime);

    // IDA 0x56e280.
    [[nodiscard]] static std::uint32_t ConstructBase12(
        std::uint32_t object, IScriptObjectFactoryRuntime& runtime);
};

}  // namespace pal4
