#pragma once

#include "pal4/script_cache_context.h"

#include <functional>
#include <vector>

namespace pal4 {
class NativeScriptFrameRunner;
struct NativeScriptCacheEntry {
    NativeScriptFrameRunner* runner{};
    std::function<void()> clear_handlers;
};

// Adapts the shared game cache policy to public SDK contexts. Native owners
// retain stable reusable contexts until their borrowed wait adapters retire.
class NativeScriptCacheContext final : public IScriptCacheContextRuntime {
public:
    using Collector = std::function<std::vector<NativeScriptCacheEntry>()>;
    NativeScriptCacheContext(Collector collect, std::function<void()> retire);
    void SetHandlerCleanup(std::function<void()> cleanup);
    std::uint32_t ScriptExecutor() override;
    std::int32_t EntryCount(std::uint32_t cache) override;
    std::int32_t RemoveExceptCurrent(std::uint32_t cache) override;
private:
    void RequireCache(std::uint32_t cache) const;
    Collector collect_;
    std::function<void()> retire_, cleanup_;
};
} // namespace pal4
