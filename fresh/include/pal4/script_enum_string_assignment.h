#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

struct ScriptEnumStringState {
    std::uint32_t status{};
    StringBufferState text{};
};
static_assert(sizeof(ScriptEnumStringState) == 16u);

class IScriptEnumStringRuntime {
public:
    virtual ~IScriptEnumStringRuntime() = default;
    [[noreturn]] virtual void ThrowRange() = 0;
    [[nodiscard]] virtual std::uint32_t DetachAndReset(
        ScriptEnumStringState& state) = 0;
    [[nodiscard]] virtual bool AllocateText(ScriptEnumStringState& state,
                                            std::uint32_t length,
                                            std::uint8_t mode) = 0;
    virtual void SetTextSize(ScriptEnumStringState& state,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t length) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptEnumStringRuntimeStub final : public IScriptEnumStringRuntime {
public:
    [[noreturn]] void ThrowRange() override;
    std::uint32_t DetachAndReset(ScriptEnumStringState& state) override;
    bool AllocateText(ScriptEnumStringState& state, std::uint32_t length,
                      std::uint8_t mode) override;
    void SetTextSize(ScriptEnumStringState& state,
                     std::uint32_t length) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override;
    void Free(std::uint32_t address) override;
};

class ScriptEnumStringAssignment final {
public:
    [[nodiscard]] static std::uint8_t Assign(
        ScriptEnumStringState& destination,
        const ScriptEnumStringState& source,
        IScriptEnumStringRuntime& runtime);
};

}  // namespace pal4
