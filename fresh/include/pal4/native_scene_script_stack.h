#pragma once

#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_frame_runner.h"
#include "pal4/native_script_cache_context.h"
#include "pal4/native_script_push_order.h"

#include <memory>
#include <string_view>
#include <vector>

namespace pal4 {

// Game stack policy over public SDK contexts. Entries stay at stable addresses
// while scene-owned callback/wait adapters are replaced across Loading.
class NativeSceneScriptStack final {
public:
    explicit NativeSceneScriptStack(NativeAngelScriptEngine& engine,
        NativeScriptPushOrder* push_order = nullptr);
    ~NativeSceneScriptStack();
    NativeSceneScriptStack(const NativeSceneScriptStack&) = delete;
    NativeSceneScriptStack& operator=(const NativeSceneScriptStack&) = delete;

    NativeScriptFrameRunner& Runner() const noexcept;
    NativeGameScriptCallbacks& Callbacks() const noexcept;
    std::size_t Depth() const noexcept { return entries_.size(); }
    bool ContainsActiveContext(const asIScriptContext* context) const noexcept;
    // IDA7DF2C0: a full function already on the stack is not pushed twice.
    // IDA7DEE60: missing functions return false without changing prior waits/context.
    bool Queue(std::string_view module, std::string_view function);
    // Pop only after the host has retired the completed entry's wait owners.
    bool PopFinished();
    void DetachServices(NativeGameScriptServices empty_services);
    void Cancel(NativeGameScriptServices empty_services);
    void ClearExecution();
    void AppendCacheEntries(std::vector<NativeScriptCacheEntry>& entries);
    bool HasPendingEntries() const noexcept;
    std::uint64_t SelectedPushOrder() const noexcept;

private:
    struct Entry;
    NativeAngelScriptEngine& engine_;
    NativeScriptPushOrder own_push_order_;
    NativeScriptPushOrder& push_order_;
    std::vector<std::unique_ptr<Entry>> entries_;
};

} // namespace pal4
