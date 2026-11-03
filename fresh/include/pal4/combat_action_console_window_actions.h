#pragma once

#include <cstdint>

namespace pal4 {

class ICombatActionDialogRuntime {
public:
    virtual ~ICombatActionDialogRuntime()=default;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t DialogDataFromParam(std::uint32_t)=0;
    virtual std::uint32_t UpdateVisibility(std::uint32_t,std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t FrameManager()=0;
    virtual void ShowTutorial(std::uint32_t,std::uint32_t)=0;
};

class ICombatActionCommandRuntime {
public:
    virtual ~ICombatActionCommandRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t BuildActionPayload(std::uint32_t)=0;
    virtual std::uint32_t EventRouter()=0;
    virtual std::uint32_t ProcessEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)=0;
};
class ICombatActionClickRuntime : public ICombatActionCommandRuntime {
public:
    ~ICombatActionClickRuntime() override=default;
    virtual void InvokeAction(std::uint32_t,std::uint32_t)=0;
    virtual void SetupSelector(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)=0;
};

class ICombatActionConsoleWindowActionRuntime : public ICombatActionDialogRuntime,public ICombatActionClickRuntime {
public:
    virtual ~ICombatActionConsoleWindowActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void InvokeAction(std::uint32_t window,
                              std::uint32_t callback) = 0;
    virtual void SetupSelector(std::uint32_t member,
                               std::uint32_t window,
                               std::uint32_t callback,
                               std::uint32_t context,
                               std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t DialogDataFromParam(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateVisibility(
        std::uint32_t window, std::uint32_t context,
        std::uint32_t visible) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ShowTutorial(std::uint32_t manager,
                              std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ResetSelector(
        std::uint32_t member, std::uint32_t first,
        std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ValidateMagic(
        std::uint32_t selection) = 0;
    [[nodiscard]] virtual std::uint32_t ValidateStunt(
        std::uint32_t selection) = 0;
    [[nodiscard]] virtual std::uint32_t BuildActionPayload(
        std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEvent(
        std::uint32_t router, std::uint32_t key,
        std::uint32_t payload, std::uint32_t flags) = 0;
};

class CombatActionConsoleWindowActions final {
public:
    // IDA 0x541480/0x541570/0x5415a0.
    [[nodiscard]] static std::uint8_t HandleActionEvent(
        std::uint32_t window, std::uint32_t event,
        ICombatActionClickRuntime& runtime);
    static void ShowDialog(std::uint32_t window, std::uint32_t event,
                           ICombatActionDialogRuntime& runtime);
    [[nodiscard]] static std::uint32_t HideDialog(
        std::uint32_t window, ICombatActionDialogRuntime& runtime);
    // IDA 0x541910-0x541b90.
    [[nodiscard]] static std::uint32_t SelectMagic(
        std::uint32_t window, std::uint32_t selection,
        ICombatActionConsoleWindowActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectStunt(
        std::uint32_t window, std::uint32_t selection,
        ICombatActionConsoleWindowActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectArticle(
        std::uint32_t window, std::uint32_t selection,
        ICombatActionConsoleWindowActionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Attack(
        std::uint32_t window, ICombatActionCommandRuntime& runtime);
    [[nodiscard]] static std::uint32_t Defend(
        std::uint32_t window, ICombatActionCommandRuntime& runtime);
    [[nodiscard]] static std::uint32_t Flee(
        std::uint32_t window, ICombatActionCommandRuntime& runtime);
};

}  // namespace pal4
