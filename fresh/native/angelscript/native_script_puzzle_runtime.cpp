#include "pal4/native_script_puzzle_runtime.h"
#include <stdexcept>
#include <utility>

static_assert(sizeof(void*) == 4);

namespace pal4 {

NativeScriptPuzzleRuntime::NativeScriptPuzzleRuntime(asIScriptContext& context,
    NativeScriptPuzzleServices services) : context_(context), services_(std::move(services)) {
    if (!services_.manager || !services_.clear_result || !services_.result ||
        !services_.enter || !services_.current_state || !services_.state_id)
        throw std::invalid_argument("Puzzle requires live UI/result/state services");
}
void NativeScriptPuzzleRuntime::RequireExecuting() const {
    if (asGetActiveContext() != &context_ || active_)
        throw std::logic_error("Puzzle requires its executing SDK context");
}
std::uint32_t NativeScriptPuzzleRuntime::DocumentAddress() const {
    return reinterpret_cast<std::uint32_t>(&document_);
}
void NativeScriptPuzzleRuntime::RequireDocument(std::uint32_t document) const {
    if (!document_live_ || document != DocumentAddress())
        throw std::invalid_argument("Puzzle temporary document owner");
}
void NativeScriptPuzzleRuntime::ClearPuzzleUiFlag() {
    RequireExecuting();
    services_.clear_result();
}
std::uint32_t NativeScriptPuzzleRuntime::CreateDocument() {
    RequireExecuting();
    if (document_live_) throw std::logic_error("Puzzle document is already live");
    document_ = {};
    document_live_ = true;
    return DocumentAddress();
}
void NativeScriptPuzzleRuntime::UpsertIntAttribute(std::uint32_t document,
    std::string_view key, std::int32_t value) {
    RequireExecuting();
    RequireDocument(document);
    if (key == "Mode") { document_.mode = value; document_.has_mode = true; }
    else if (key == "Type") { document_.type = value; document_.has_type = true; }
    else throw std::invalid_argument("Unknown puzzle document attribute");
}
void NativeScriptPuzzleRuntime::EnterState(std::uint32_t state, std::uint32_t document) {
    RequireExecuting();
    RequireDocument(document);
    if (state != 8u || !document_.has_mode || !document_.has_type)
        throw std::invalid_argument("Puzzle state8 requires Mode/Type");
    services_.enter(document_.mode, document_.type);
}
PuzzleWaitState* NativeScriptPuzzleRuntime::AllocateWait(std::uint32_t bytes) {
    RequireExecuting();
    if (bytes != sizeof(wait_)) throw std::invalid_argument("Puzzle wait size");
    wait_ = {};
    return &wait_;
}
std::uint32_t NativeScriptPuzzleRuntime::ScriptExecutor() {
    return reinterpret_cast<std::uint32_t>(this);
}
void NativeScriptPuzzleRuntime::ConstructWait(PuzzleWaitState& wait,
    std::uint8_t mode, std::uint32_t queue) {
    RequireExecuting();
    if (&wait != &wait_ || mode != 1u || queue != ScriptExecutor() + ScriptPuzzleGame::kQueueOffset)
        throw std::invalid_argument("Puzzle wait queue owner");
}
void NativeScriptPuzzleRuntime::FlushWait(PuzzleWaitState& wait) {
    RequireExecuting();
    if (&wait != &wait_ || wait.vtable_address != ScriptPuzzleGame::kWaitVtable)
        throw std::invalid_argument("Puzzle wait identity");
}
void NativeScriptPuzzleRuntime::AddHandler(std::uint32_t queue, PuzzleWaitState* wait) {
    RequireExecuting();
    if (queue != ScriptExecutor() + ScriptPuzzleGame::kQueueOffset || wait != &wait_ ||
        wait_.vtable_address != ScriptPuzzleGame::kWaitVtable || context_.Suspend() < 0)
        throw std::runtime_error("Puzzle cannot suspend its SDK context");
    active_ = true;
}
std::uint32_t NativeScriptPuzzleRuntime::DestroyDocument(std::uint32_t document) {
    RequireDocument(document);
    document_ = {};
    document_live_ = false;
    return document;
}
std::uint8_t NativeScriptPuzzleRuntime::ReadByte(std::uint32_t address) {
    if (address != UiFrameManager() + 0x222u)
        throw std::out_of_range("Puzzle result must read UIFrameManager+222");
    return services_.result();
}
bool NativeScriptPuzzleRuntime::Tick(float) {
    if (!active_ || !ScriptCurrentStateWait::Complete(*this)) return false;
    active_ = false;
    return true;
}
void NativeScriptPuzzleRuntime::ClearHandlers() {
    if (asGetActiveContext() == &context_)
        throw std::logic_error("Cannot retire executing puzzle wait");
    active_ = false;
    wait_ = {};
    document_live_ = false;
    document_ = {};
}

} // namespace pal4
