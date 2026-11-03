#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptSkillVector8Runtime {
public:
    virtual ~IScriptSkillVector8Runtime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptSkillVector8 final {
public:
    // IDA 0x589da0. The vector header stores begin/end/capacity at +4/+8/+12.
    [[nodiscard]] static std::uint32_t Insert(
        std::uint32_t vector_header, std::uint32_t position,
        std::uint32_t count, const std::array<std::uint32_t, 2>& item,
        IScriptSkillVector8Runtime& runtime);
};

}  // namespace pal4
