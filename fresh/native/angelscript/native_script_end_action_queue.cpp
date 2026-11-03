#include "pal4/native_script_end_action_queue.h"

#include <algorithm>
#include <stdexcept>

static_assert(sizeof(void*) == sizeof(std::uint32_t));

namespace pal4 {

NativeScriptEndActionQueue::NativeScriptEndActionQueue() {
    state_.current_stack_record = reinterpret_cast<std::uint32_t>(&record_);
    record_.function_index = 1;
}

void NativeScriptEndActionQueue::Append(std::unique_ptr<PlayerEndActionWaitState> task) {
    if (!task || update_) throw std::logic_error("end-action queue append during dispatch or null task");
    auto entry = std::make_unique<Entry>();
    (void)ScriptWaitHandlerBase::Initialize(entry->base, 1u, &state_, nullptr);
    entry->task = std::move(task);
    handlers_.reserve(handlers_.size() + 1u);
    entries_.push_back(std::move(entry));
    handlers_.push_back(&entries_.back()->base);
    (void)ScriptExecutionContext::SetExecutionFlag(state_);
}

void NativeScriptEndActionQueue::AppendObservedHandler(
    const void* identity, std::uint32_t vtable, std::uint8_t mode,
    std::function<bool()> poll_closed) {
    if (!identity || !poll_closed || update_)
        throw std::logic_error("observed wait requires a stable owner outside dispatch");
    for (const auto& entry : entries_)
        if (entry->observed == identity)
            throw std::logic_error("observed wait already registered");
    auto entry = std::make_unique<Entry>();
    (void)ScriptWaitHandlerBase::Initialize(entry->base, mode, &state_,
        reinterpret_cast<const void*>(vtable));
    entry->observed = identity;
    entry->poll_closed = std::move(poll_closed);
    handlers_.reserve(handlers_.size() + 1u);
    entries_.push_back(std::move(entry));
    handlers_.push_back(&entries_.back()->base);
    if (mode != 0u) (void)ScriptExecutionContext::SetExecutionFlag(state_);
}

void NativeScriptEndActionQueue::RemoveObservedHandler(const void* identity) noexcept {
    std::erase_if(handlers_, [&](auto* base) { return Find(*base).observed == identity; });
    std::erase_if(entries_, [&](const auto& entry) { return entry->observed == identity; });
    if (Empty()) (void)ScriptExecutionContext::ClearExecutionFlag(state_);
}

void NativeScriptEndActionQueue::Close(PlayerEndActionWaitState& task) {
    for (auto& entry : entries_)
        if (entry->task.get() == &task) {
            ScriptWaitHandlerBase::Close(entry->base);
            return;
        }
    throw std::logic_error("end-action close requires its creating queue");
}

void NativeScriptEndActionQueue::AppendCompletedHandler(
    std::uint32_t vtable, std::int32_t marker, std::string_view name) {
    if (update_ || marker < 0)
        throw std::logic_error("completed wait registration during dispatch or unnamed marker");
    auto entry = std::make_unique<Entry>();
    (void)ScriptWaitHandlerBase::Initialize(entry->base, 0u, &state_,
        reinterpret_cast<const void*>(vtable));
    entry->base.marker = marker;
    entry->name = name;
    ScriptWaitHandlerBase::Close(entry->base);
    handlers_.reserve(handlers_.size() + 1u);
    entries_.reserve(entries_.size() + 1u);
    const auto previous_size = handlers_.size();
    handlers_.resize(previous_size + 1u);
    entries_.push_back(std::move(entry));
    state_.buffer_36 = handlers_.data();
    state_.buffer_40 = handlers_.data() + previous_size;
    state_.buffer_44 = handlers_.data() + handlers_.size();
    class Registration final : public IScriptHandlerRegistrationRuntime {
    public:
        explicit Registration(NativeScriptEndActionQueue& queue) : queue_(queue) {}
        bool SameHandlerKey(const ScriptWaitHandlerBaseState& incoming,
                            const ScriptWaitHandlerBaseState& existing) override {
            return queue_.Find(const_cast<ScriptWaitHandlerBaseState&>(incoming)).name ==
                   queue_.Find(const_cast<ScriptWaitHandlerBaseState&>(existing)).name;
        }
        void ResetHandler(ScriptWaitHandlerBaseState&) override {}
        void DeleteHandler(ScriptWaitHandlerBaseState& base, unsigned flags) override {
            queue_.Delete(base, flags);
        }
        void* AllocatePointers(std::size_t) override {
            throw std::logic_error("completed wait registration exceeded reserved pointers");
        }
        void FreePointers(void*) override {
            throw std::logic_error("completed wait registration replaced borrowed pointers");
        }
    private:
        NativeScriptEndActionQueue& queue_;
    } registration(*this);
    (void)ScriptExecutionContext::AddWaitHandler(
        state_, entries_.back()->base, registration);
    handlers_.resize(static_cast<ScriptWaitHandlerBaseState**>(state_.buffer_40) -
                     handlers_.data());
    std::erase_if(entries_, [](const auto& old) { return old->retired; });
    // A closed nonblocking record never sets the execution flag. Its removal
    // as the tail clears that flag through the original shared dispatcher.
}

NativeScriptEndActionQueue::Entry& NativeScriptEndActionQueue::Find(
    ScriptWaitHandlerBaseState& base) {
    for (auto& entry : entries_)
        if (&entry->base == &base) return *entry;
    throw std::logic_error("end-action shared dispatch target");
}

void NativeScriptEndActionQueue::Process(ScriptWaitHandlerBaseState& base, int, int) {
    if (!update_) throw std::logic_error("end-action update callback is unavailable");
    auto& entry = Find(base);
    if (entry.task) (*update_)(*entry.task);
    if (entry.poll_closed && entry.poll_closed()) ScriptWaitHandlerBase::Close(base);
}

void NativeScriptEndActionQueue::Delete(ScriptWaitHandlerBaseState& base, unsigned flags) {
    if (flags != 1u) throw std::logic_error("end-action shared deletion flags");
    Find(base).retired = true;
}

int NativeScriptEndActionQueue::ExecuteCurrentModule(ScriptExecutionContextState&) {
    // The enclosing native frame runner owns SDK execution. The shared
    // dispatcher has already applied the original execution-flag change.
    return 0;
}

void NativeScriptEndActionQueue::FinishDispatch() {
    handlers_.resize(static_cast<ScriptWaitHandlerBaseState**>(state_.buffer_40) -
                     handlers_.data());
    std::erase_if(entries_, [](const auto& entry) { return entry->retired; });
    update_ = nullptr;
}

bool NativeScriptEndActionQueue::Process(
    const std::function<void(PlayerEndActionWaitState&)>& update) {
    if (Empty()) return false;
    if (update_ || !update) throw std::logic_error("end-action nested or missing dispatch");
    const bool blocked = BlocksScript();
    state_.buffer_36 = handlers_.data();
    state_.buffer_40 = handlers_.data() + handlers_.size();
    update_ = &update;
    try {
        (void)ScriptExecutionContext::ProcessWaitHandlers(state_, 0, 0, *this);
    } catch (...) {
        FinishDispatch();
        throw;
    }
    FinishDispatch();
    return blocked && !BlocksScript();
}

void NativeScriptEndActionQueue::Visit(
    const std::function<void(const PlayerEndActionWaitState&)>& visit) const {
    for (const auto& entry : entries_) if (entry->task) visit(*entry->task);
}

void NativeScriptEndActionQueue::EraseIf(
    const std::function<bool(const PlayerEndActionWaitState&)>& predicate) {
    if (update_) throw std::logic_error("end-action removal during dispatch");
    std::erase_if(handlers_, [&](auto* base) {
        const auto& entry = Find(*base);
        return entry.task && predicate(*entry.task);
    });
    std::erase_if(entries_, [&](const auto& entry) {
        return entry->task && predicate(*entry->task);
    });
    if (Empty()) (void)ScriptExecutionContext::ClearExecutionFlag(state_);
}

void NativeScriptEndActionQueue::Clear() noexcept {
    handlers_.clear();
    entries_.clear();
    record_.execution_flag = 0u;
    update_ = nullptr;
}

}  // namespace pal4
