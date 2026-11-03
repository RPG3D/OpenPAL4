#pragma once

#include "pal4/script_talk_handler.h"
#include "pal4/select_dialog_option_records.h"

#include <cstdint>

namespace pal4 {

class IScriptRandomTalkService {
public:
    virtual ~IScriptRandomTalkService() = default;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    [[nodiscard]] virtual ScenarioDialogWaitState* AllocateHandler(
        std::uint32_t size) = 0;
    virtual void ConstructBase(ScenarioDialogWaitState& handler,
                               std::uint32_t wait_mode,
                               std::uint32_t queue_address) = 0;
    virtual void Flush(ScenarioDialogWaitState& handler) = 0;
    virtual void AddHandler(std::uint32_t queue_address,
                            ScenarioDialogWaitState* handler) = 0;
    [[nodiscard]] virtual std::int32_t Random() = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateScenarioDialog(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint8_t IsVisible(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual std::uint8_t SetTextRecord(
        std::uint32_t dialog, std::uint32_t record,
        std::uint32_t display_value) = 0;
};

class ScriptRandomTalkServiceStub final : public IScriptRandomTalkService {
public:
    std::uint32_t ScriptExecutor() override;
    ScenarioDialogWaitState* AllocateHandler(std::uint32_t) override;
    void ConstructBase(ScenarioDialogWaitState&, std::uint32_t,
                       std::uint32_t) override;
    void Flush(ScenarioDialogWaitState&) override;
    void AddHandler(std::uint32_t, ScenarioDialogWaitState*) override;
    std::int32_t Random() override;
    std::uint32_t UiFrameManager() override;
    std::uint32_t CreateScenarioDialog(std::uint32_t) override;
    std::uint8_t IsVisible(std::uint32_t) override;
    std::uint8_t SetTextRecord(std::uint32_t, std::uint32_t,
                               std::uint32_t) override;
};

// Read-only candidate view separates game selection policy from legacy storage.
class IScriptRandomTalkPoolView {
public:
    virtual ~IScriptRandomTalkPoolView() = default;
    virtual std::uint32_t Count() = 0;
    virtual std::uint32_t Record(std::uint32_t index) = 0;
};

class ScriptRandomTalk final {
public:
    static constexpr std::uint32_t kArrayOffset = 0x10u;
    static constexpr std::uint32_t kRecordSize = 0x10u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;
    static constexpr std::uint32_t kHandlerSize = 0x20u;
    static constexpr std::uint32_t kVtable = 0x849520u;
    static constexpr std::uint32_t kDisplayValue = 0x3dcccccdu;

    ScriptRandomTalk(ISelectDialogOptionArrayRuntime& records,
                     IScriptRandomTalkService& service)
        : records_(records), service_(service) {}

    // IDA 0x5dfd30, 0x5dfee0 and 0x5dffc0.
    [[nodiscard]] std::uint32_t Push(std::uint32_t source_record);
    [[nodiscard]] std::uint8_t Show();
    [[nodiscard]] static std::uint8_t Show(
        IScriptRandomTalkPoolView& pool, IScriptRandomTalkService& service);
    [[nodiscard]] std::uint32_t Release();

private:
    ISelectDialogOptionArrayRuntime& records_;
    IScriptRandomTalkService& service_;
};

}  // namespace pal4
