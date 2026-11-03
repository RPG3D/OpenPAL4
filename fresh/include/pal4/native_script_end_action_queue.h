#pragma once

#include "pal4/script_execution_context.h"
#include "pal4/script_player_action.h"
#include "pal4/script_wait_handler_base.h"
#include "pal4/script_completed_handler_queue.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pal4 {

// Owns end-action dispatch records and the context's completed position/angle
// records, plus camera-owned observed waits. Shared 7DF4B0 retains unfinished
// earlier records when a closed tail clears the execution flag; completed
// siblings and camera waits must preserve their cross-family order.
class NativeScriptEndActionQueue final : public IScriptCompletedHandlerQueue,
                                       private IScriptHandlerDispatchRuntime {
public:
    NativeScriptEndActionQueue();
    NativeScriptEndActionQueue(const NativeScriptEndActionQueue&) = delete;
    NativeScriptEndActionQueue& operator=(const NativeScriptEndActionQueue&) = delete;
    NativeScriptEndActionQueue(NativeScriptEndActionQueue&&) = delete;
    NativeScriptEndActionQueue& operator=(NativeScriptEndActionQueue&&) = delete;
    void Append(std::unique_ptr<PlayerEndActionWaitState> task);
    void AppendCompletedHandler(std::uint32_t vtable, std::int32_t marker,
                                std::string_view name) override;
    // Other context wait owners retain their state. Polling and tail retirement
    // share the same original dispatcher as end actions and completed setters.
    void AppendObservedHandler(const void* identity, std::uint32_t vtable,
        std::uint8_t mode, std::function<bool()> poll_closed);
    void RemoveObservedHandler(const void* identity) noexcept;
    void Close(PlayerEndActionWaitState& task);
    [[nodiscard]] bool Process(
        const std::function<void(PlayerEndActionWaitState&)>& update);
    void Visit(const std::function<void(const PlayerEndActionWaitState&)>& visit) const;
    void EraseIf(const std::function<bool(const PlayerEndActionWaitState&)>& predicate);
    void Clear() noexcept;
    [[nodiscard]] bool Empty() const noexcept { return handlers_.empty(); }
    [[nodiscard]] std::size_t Size() const noexcept { return handlers_.size(); }
    [[nodiscard]] bool BlocksScript() const noexcept { return record_.execution_flag != 0; }

private:
    struct Entry {
        ScriptWaitHandlerBaseState base{};
        std::unique_ptr<PlayerEndActionWaitState> task;
        const void* observed{};
        std::function<bool()> poll_closed;
        bool retired{};
        std::string name;
    };
    Entry& Find(ScriptWaitHandlerBaseState& base);
    void FinishDispatch();
    void Process(ScriptWaitHandlerBaseState&, int, int) override;
    void Delete(ScriptWaitHandlerBaseState&, unsigned) override;
    int ExecuteCurrentModule(ScriptExecutionContextState&) override;
    ScriptExecutionContextState state_{};
    ScriptStackModuleRecord record_{};
    std::vector<std::unique_ptr<Entry>> entries_;
    std::vector<ScriptWaitHandlerBaseState*> handlers_;
    const std::function<void(PlayerEndActionWaitState&)>* update_{};
};

}  // namespace pal4
