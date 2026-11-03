#include "pal4/combat_action_console_window_actions.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kActionEventKey = 0x8b62d0u;

std::uint32_t SendAction(const std::uint32_t context,
                         const std::uint32_t action,
                         const std::uint32_t selection_address,
                         ICombatActionCommandRuntime& runtime) {
    const auto payload = runtime.BuildActionPayload(context);
    if (selection_address != 0u)
        runtime.WriteWord(payload + 8u,
                          runtime.ReadWord(selection_address));
    runtime.WriteWord(payload + 12u, action);
    return runtime.ProcessEvent(runtime.EventRouter(), kActionEventKey,
                                payload, 1u);
}

std::uint32_t ResetAll(const std::uint32_t window,
                       ICombatActionConsoleWindowActionRuntime& runtime) {
    (void)runtime.ResetSelector(window + 0x5cu, 0u, 0u);
    (void)runtime.ResetSelector(window + 0x110u, 0u, 0u);
    return runtime.ResetSelector(window + 0xb4u, 0u, 0u);
}

}  // namespace

std::uint8_t CombatActionConsoleWindowActions::HandleActionEvent(
    const std::uint32_t window, const std::uint32_t event,
    ICombatActionClickRuntime& runtime) {
    const auto context = runtime.ReadWord(window + 0x58u);
    if (context == 0u) return 1u;
    const auto payload = runtime.ReadWord(event + 8u);
    const auto type = runtime.ReadWord(payload + 0x118u);
    runtime.WriteWord(window + 0x1e4u,
                      window + 0x168u + 20u * type);
    switch (type) {
    case 0u:
        runtime.InvokeAction(window, 0x541b10u);
        break;
    case 1u:
        runtime.InvokeAction(window, 0x541b50u);
        break;
    case 2u:
        runtime.SetupSelector(window + 0x110u, window, 0x5419c0u,
                              context, 0u);
        break;
    case 3u:
        runtime.SetupSelector(window + 0x5cu, window, 0x541910u,
                              context, 0u);
        break;
    case 4u:
        runtime.SetupSelector(window + 0xb4u, window, 0x541a70u,
                              context, 0u);
        break;
    case 5u:
        runtime.InvokeAction(window, 0x541b90u);
        break;
    default:
        break;
    }
    return 1u;
}

void CombatActionConsoleWindowActions::ShowDialog(
    const std::uint32_t window, const std::uint32_t event,
    ICombatActionDialogRuntime& runtime) {
    const auto context = runtime.DialogDataFromParam(event);
    runtime.WriteWord(window + 0x58u, context);
    (void)runtime.UpdateVisibility(window, context, 1u);
    runtime.ShowTutorial(runtime.FrameManager(), 9u);
}

std::uint32_t CombatActionConsoleWindowActions::HideDialog(
    const std::uint32_t window,
    ICombatActionDialogRuntime& runtime) {
    runtime.WriteWord(window + 0x58u, 0u);
    return runtime.UpdateVisibility(window, 0u, 0u);
}

std::uint32_t CombatActionConsoleWindowActions::SelectMagic(
    const std::uint32_t window, const std::uint32_t selection,
    ICombatActionConsoleWindowActionRuntime& runtime) {
    (void)ResetAll(window, runtime);
    if (selection == 0u)
        return runtime.ResetSelector(window + 0x5cu, 0u, 0u);
    const auto valid = runtime.ValidateMagic(selection);
    if (valid == 0u) return valid;
    const auto context = runtime.ReadWord(window + 0x58u);
    if (context == 0u) return valid;
    return SendAction(context, 0u, selection, runtime);
}

std::uint32_t CombatActionConsoleWindowActions::SelectStunt(
    const std::uint32_t window, const std::uint32_t selection,
    ICombatActionConsoleWindowActionRuntime& runtime) {
    (void)ResetAll(window, runtime);
    if (selection == 0u)
        return runtime.ResetSelector(window + 0x110u, 0u, 0u);
    const auto valid = runtime.ValidateStunt(selection);
    if (valid == 0u) return valid;
    const auto context = runtime.ReadWord(window + 0x58u);
    if (context == 0u) return valid;
    return SendAction(context, 1u, selection, runtime);
}

std::uint32_t CombatActionConsoleWindowActions::SelectArticle(
    const std::uint32_t window, const std::uint32_t selection,
    ICombatActionConsoleWindowActionRuntime& runtime) {
    const auto reset_result = ResetAll(window, runtime);
    if (selection == 0u)
        return runtime.ResetSelector(window + 0xb4u, 0u, 0u);
    const auto context = runtime.ReadWord(window + 0x58u);
    if (context == 0u) return reset_result;
    return SendAction(context, 2u, selection, runtime);
}

std::uint32_t CombatActionConsoleWindowActions::Attack(
    const std::uint32_t window,
    ICombatActionCommandRuntime& runtime) {
    return SendAction(runtime.ReadWord(window + 0x58u), 4u, 0u, runtime);
}

std::uint32_t CombatActionConsoleWindowActions::Defend(
    const std::uint32_t window,
    ICombatActionCommandRuntime& runtime) {
    return SendAction(runtime.ReadWord(window + 0x58u), 5u, 0u, runtime);
}

std::uint32_t CombatActionConsoleWindowActions::Flee(
    const std::uint32_t window,
    ICombatActionCommandRuntime& runtime) {
    return SendAction(runtime.ReadWord(window + 0x58u), 3u, 0u, runtime);
}

}  // namespace pal4
