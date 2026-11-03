#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace pal4 {

struct ScriptCommandRecord {
    std::uint32_t command;
    std::uint32_t argument;
    std::uint32_t reserved;
};
static_assert(sizeof(ScriptCommandRecord) == 12u);

class ScriptCommandQueue final {
public:
    // IDA 0x5d4060 appends to the palScriptExe +4/+8/+12 vector.
    [[nodiscard]] ScriptCommandRecord* Push(std::uint32_t command,
                                            std::uint32_t argument);
    [[nodiscard]] const std::vector<ScriptCommandRecord>& Commands() const {
        return commands_;
    }
    [[nodiscard]] std::size_t Capacity() const { return commands_.capacity(); }
    void Clear() noexcept { commands_.clear(); }

private:
    std::vector<ScriptCommandRecord> commands_;
};

class IScriptCommandRuntime {
public:
    virtual ~IScriptCommandRuntime() = default;
    [[nodiscard]] virtual ScriptCommandQueue& ScriptExecutorQueue() = 0;
};

class ScriptCommandDispatcher final {
public:
    // IDA 0x5d4060 singleton entrypoint.
    [[nodiscard]] static ScriptCommandRecord* Push(
        std::uint32_t command, std::uint32_t argument,
        IScriptCommandRuntime& runtime);
};

}  // namespace pal4
