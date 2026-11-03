#pragma once

#include "pal4/native_script_frame_wait_runtime.h"
#include "pal4/script_talk_handler.h"
#include "pal4/script_scenario_portrait.h"

#include <angelscript.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace pal4 {

class NativeCeguiUiSession;

class NativeScriptTalkRuntime final : public IScriptTalkHandlerRuntime,
                                      public IScriptScenarioPortraitRuntime,
                                      public INativeScriptFrameWaitRuntime {
public:
    using VoiceFlagReader = std::function<std::uint8_t()>;
    using VoicePlayer = std::function<std::uint32_t(std::string_view)>;

    NativeScriptTalkRuntime(NativeCeguiUiSession& ui,
                            asIScriptContext& context,
                            VoiceFlagReader voice_flag,
                            VoicePlayer play_voice = {});

    [[nodiscard]] bool Waiting() const noexcept { return !handlers_.empty(); }
    void ClearHandlers();
    [[nodiscard]] bool Tick(float elapsed) override;
    [[nodiscard]] std::uint32_t VoiceHandle() const noexcept {
        return voice_handle_;
    }

    std::uint8_t VoiceFlag() override;
    std::uint32_t ScriptExecutor() override;
    std::uint32_t AudioManager() override;
    std::uint32_t PlayMusic(std::uint32_t manager, std::string_view path,
                            std::uint32_t mode) override;
    void StoreVoiceHandle(std::uint32_t address,
                          std::uint32_t handle) override;
    ScenarioDialogWaitState* Allocate(std::uint32_t size) override;
    void ConstructBase(ScenarioDialogWaitState& handler,
                       std::uint32_t wait_mode,
                       std::uint32_t queue_address) override;
    void Flush(ScenarioDialogWaitState& handler) override;
    std::uint32_t AddHandler(std::uint32_t queue_address,
                             ScenarioDialogWaitState* handler) override;
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateScenarioDialog(std::uint32_t manager) override;
    std::uint8_t IsVisible(std::uint32_t dialog) override;
    std::uint8_t SetText(std::uint32_t dialog, std::string_view text,
                         std::uint32_t display_value) override;
    void HideDialog(std::uint32_t dialog, std::uint32_t first,
                    std::uint32_t second) override;
    void SetClosedFlag(ScenarioDialogWaitState& handler) override;
    std::uint32_t ScenarioDialog(std::uint32_t manager) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint8_t SetPortrait(std::uint32_t dialog, std::uint32_t name,
                             std::int32_t argument) override;

private:
    struct Handler {
        std::unique_ptr<ScenarioDialogWaitState> state;
        bool closed{};
    };

    [[nodiscard]] std::uint32_t QueueAddress() const noexcept;

    NativeCeguiUiSession& ui_;
    asIScriptContext& context_;
    VoiceFlagReader voice_flag_;
    VoicePlayer play_voice_;
    ScriptTalkHandler talk_;
    std::unique_ptr<ScenarioDialogWaitState> pending_;
    std::vector<Handler> handlers_;
    std::uint32_t voice_handle_{};
};

}  // namespace pal4
