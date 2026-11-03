#pragma once

#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_current_state_wait.h"
#include "pal4/script_puzzle_game.h"
#include "pal4/script_ui_state_callbacks.h"
#include <angelscript.h>
#include <functional>

namespace pal4 {

struct NativeScriptPuzzleServices {
    std::uint32_t manager{};
    std::function<void()> clear_result;
    std::function<std::uint8_t()> result;
    std::function<void(std::int32_t mode, std::int32_t type)> enter;
    std::function<std::uint32_t()> current_state;
    std::function<std::int32_t(std::uint32_t)> state_id;
};

// One SDK context owns its wait and temporary Mode/Type document. All UI and
// game-state endpoints are borrowed; retiring a wait never closes global UI.
class NativeScriptPuzzleRuntime final : public IScriptPuzzleRuntime,
    public IScriptPuzzleResultRuntime, public IScriptCurrentStateWaitRuntime,
    public INativeScriptHandlerWaitRuntime {
public:
    NativeScriptPuzzleRuntime(asIScriptContext&, NativeScriptPuzzleServices);
    bool BlocksScript() const override { return active_; }
    bool Tick(float) override;
    void ClearHandlers();
    bool HasDocument() const noexcept { return document_live_; }

    void ClearPuzzleUiFlag() override;
    std::uint32_t CreateDocument() override;
    void UpsertIntAttribute(std::uint32_t, std::string_view, std::int32_t) override;
    void EnterState(std::uint32_t, std::uint32_t) override;
    PuzzleWaitState* AllocateWait(std::uint32_t) override;
    std::uint32_t ScriptExecutor() override;
    void ConstructWait(PuzzleWaitState&, std::uint8_t, std::uint32_t) override;
    void FlushWait(PuzzleWaitState&) override;
    void AddHandler(std::uint32_t, PuzzleWaitState*) override;
    std::uint32_t DestroyDocument(std::uint32_t) override;
    std::uint32_t UiFrameManager() override { return services_.manager; }
    std::uint8_t ReadByte(std::uint32_t) override;
    std::uint32_t CurrentState() override { return services_.current_state(); }
    std::int32_t CurrentStateId(std::uint32_t state) override { return services_.state_id(state); }

private:
    void RequireExecuting() const;
    void RequireDocument(std::uint32_t) const;
    std::uint32_t DocumentAddress() const;
    asIScriptContext& context_;
    NativeScriptPuzzleServices services_;
    struct Document { std::int32_t mode{}, type{}; bool has_mode{}, has_type{}; } document_;
    PuzzleWaitState wait_{};
    bool document_live_{}, active_{};
};

} // namespace pal4
