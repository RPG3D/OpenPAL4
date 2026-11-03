#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ScenarioDialogWaitState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base_state{};
};
static_assert(sizeof(ScenarioDialogWaitState) == 0x20u);

class IScriptTalkHandlerRuntime {
public:
    virtual ~IScriptTalkHandlerRuntime() = default;
    [[nodiscard]] virtual std::uint8_t VoiceFlag() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayMusic(
        std::uint32_t manager, std::string_view path,
        std::uint32_t mode) = 0;
    virtual void StoreVoiceHandle(std::uint32_t address,
                                  std::uint32_t handle) = 0;
    [[nodiscard]] virtual ScenarioDialogWaitState* Allocate(
        std::uint32_t size) = 0;
    virtual void ConstructBase(ScenarioDialogWaitState& handler,
                               std::uint32_t wait_mode,
                               std::uint32_t queue_address) = 0;
    virtual void Flush(ScenarioDialogWaitState& handler) = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t queue_address, ScenarioDialogWaitState* handler) = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateScenarioDialog(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint8_t IsVisible(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual std::uint8_t SetText(
        std::uint32_t dialog, std::string_view text,
        std::uint32_t display_value) = 0;
    virtual void HideDialog(std::uint32_t dialog,
                            std::uint32_t first,
                            std::uint32_t second) = 0;
    virtual void SetClosedFlag(ScenarioDialogWaitState& handler) = 0;
};

class ScriptTalkHandlerRuntimeStub final : public IScriptTalkHandlerRuntime {
public:
    std::uint8_t VoiceFlag() override;
    std::uint32_t ScriptExecutor() override;
    std::uint32_t AudioManager() override;
    std::uint32_t PlayMusic(std::uint32_t, std::string_view,
                            std::uint32_t) override;
    void StoreVoiceHandle(std::uint32_t, std::uint32_t) override;
    ScenarioDialogWaitState* Allocate(std::uint32_t) override;
    void ConstructBase(ScenarioDialogWaitState&, std::uint32_t,
                       std::uint32_t) override;
    void Flush(ScenarioDialogWaitState&) override;
    std::uint32_t AddHandler(std::uint32_t,
                             ScenarioDialogWaitState*) override;
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateScenarioDialog(std::uint32_t) override;
    std::uint8_t IsVisible(std::uint32_t) override;
    std::uint8_t SetText(std::uint32_t, std::string_view,
                         std::uint32_t) override;
    void HideDialog(std::uint32_t, std::uint32_t, std::uint32_t) override;
    void SetClosedFlag(ScenarioDialogWaitState&) override;
};

class ScriptTalkHandler final {
public:
    static constexpr std::uint32_t kHandlerSize = 0x20u;
    static constexpr std::uint32_t kVtable = 0x849520u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::uint32_t kVoiceHandleOffset = 0x24u;
    static constexpr std::uint32_t kDisplayValue = 0x3dcccccdu;
    static constexpr std::string_view kVoiceDirectory =
        "gamedata\\PALSound\\";

    explicit ScriptTalkHandler(IScriptTalkHandlerRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5dfb10, 0x5dfcd0, 0x5dfd00 and 0x5e0070.
    [[nodiscard]] std::uint8_t Invoke(std::string_view text,
                                       std::string_view voice_id);
    [[nodiscard]] std::uint32_t Wait();
    void Update(ScenarioDialogWaitState& handler);
    void Close(ScenarioDialogWaitState& handler);

private:
    [[nodiscard]] std::uint32_t ScenarioDialog();

    IScriptTalkHandlerRuntime& runtime_;
};

}  // namespace pal4
