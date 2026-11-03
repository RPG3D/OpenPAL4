#pragma once

#include <cstdint>

namespace pal4 {

class IScriptCacheContextRuntime {
public:
    virtual ~IScriptCacheContextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    [[nodiscard]] virtual std::int32_t EntryCount(
        std::uint32_t cache) = 0;
    [[nodiscard]] virtual std::int32_t RemoveExceptCurrent(
        std::uint32_t cache) = 0;
};

class ScriptCacheContext final {
public:
    // IDA 0x5d7510, giScriptClearCTXButCurrent.
    [[nodiscard]] static std::int32_t ClearExceptCurrent(
        IScriptCacheContextRuntime& runtime);
};

}  // namespace pal4
