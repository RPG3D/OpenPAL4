#pragma once

#include <cstdint>

namespace pal4 {

class IScriptObjectResearchRuntime {
public:
    virtual ~IScriptObjectResearchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ScriptObjectResearch final {
public:
    // IDA 0x5d5720, giGameObjectSetResearch.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t name_payload, std::uint32_t enabled,
        IScriptObjectResearchRuntime& runtime);
};

}  // namespace pal4
