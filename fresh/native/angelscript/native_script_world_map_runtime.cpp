#include "pal4/native_script_world_map_runtime.h"
#include "pal4/game_arena_load_handler.h"

#include <stdexcept>
#include <utility>

namespace pal4 {
NativeScriptWorldMapRuntime::NativeScriptWorldMapRuntime(
    asIScriptContext& context, IGameArenaLoadRuntime& xml,
    NativeTutorialServices services)
    : context_(context), xml_(xml), wait_(context, std::move(services)) {}

std::uint32_t NativeScriptWorldMapRuntime::UiFrameManager() { return wait_.UiFrameManager(); }
void NativeScriptWorldMapRuntime::ProcessWorldMapEvent(
    std::uint32_t manager, std::uint32_t first, std::uint32_t second) {
    if (manager != UiFrameManager() || first != 1u || second != 0u ||
        asGetActiveContext() != &context_)
        throw std::invalid_argument("WorldMap show requires its invoking SDK context");
    wait_.ShowTutorial(manager, 0u);
}
std::uint32_t NativeScriptWorldMapRuntime::FindFrame(
    std::uint32_t manager, std::string_view name) {
    if (name != ScriptWorldMapWait::kFrameName)
        throw std::invalid_argument("WorldMap frame lookup identity");
    return wait_.FindFrame(manager, ScriptTutorialWait::kFrameName);
}
TutorialWaitState* NativeScriptWorldMapRuntime::AllocateWait(std::uint32_t bytes) {
    return wait_.AllocateWait(bytes);
}
std::uint32_t NativeScriptWorldMapRuntime::ScriptExecutor() { return wait_.ScriptExecutor(); }
void NativeScriptWorldMapRuntime::ConstructWait(
    TutorialWaitState& state, std::uint8_t mode, std::uint32_t queue) {
    wait_.ConstructWait(state, mode, queue);
}
void NativeScriptWorldMapRuntime::FlushWait(TutorialWaitState& state) { wait_.FlushWait(state); }
void NativeScriptWorldMapRuntime::AddHandler(std::uint32_t queue, TutorialWaitState* state) {
    (void)wait_.AddHandler(queue, state);
}
std::uint32_t NativeScriptWorldMapRuntime::CreateDocument() {
    if (document_) throw std::logic_error("WorldMap XML reentry");
    document_ = xml_.InitializeXmlDocument();
    if (!document_) throw std::runtime_error("WorldMap XML allocation failed");
    return document_;
}
void NativeScriptWorldMapRuntime::EnterState(std::uint32_t state, std::uint32_t document) {
    if (asGetActiveContext() != &context_ || state != 11u || !document_ ||
        document != document_ || !wait_.BlocksScript())
        throw std::invalid_argument("WorldMap state entry requires its live frame wait and XML");
    xml_.InitializeStateAndUi(xml_.GameInstance(), state, document);
}
std::uint32_t NativeScriptWorldMapRuntime::DestroyDocument(std::uint32_t document) {
    if (!document_ || document != document_)
        throw std::invalid_argument("WorldMap XML retirement owner");
    const auto result = static_cast<std::uint32_t>(xml_.DestroyXmlDocument(document));
    document_ = 0u;
    return result;
}
void NativeScriptWorldMapRuntime::ClearHandlers() {
    if (document_) throw std::logic_error("WorldMap XML still borrowed during retirement");
    wait_.ClearHandlers();
}
} // namespace pal4
