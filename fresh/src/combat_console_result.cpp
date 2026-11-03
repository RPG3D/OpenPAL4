#include "pal4/combat_console.h"

namespace pal4 {

std::uint32_t CombatConsoleWindow::ShowCombatResult(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsoleJointHintRuntime& runtime) {
    runtime.SetVisible(runtime.ReadWord(console + 216u), true);
    runtime.WriteWord(console + 268u, runtime.ActorFromEvent(event));
    runtime.WriteWord(console + 272u, runtime.ReadWord(event + 4u));
    runtime.WriteWord(runtime.CombatSystem() + 392u, 0u);
    const auto system = runtime.CombatSystem();
    runtime.WriteWord(system + 388u, 0u);
    return system;
}

std::uint32_t CombatConsoleWindow::HideCombatResult(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsoleResultRuntime& runtime) {
    runtime.SetVisible(runtime.ReadWord(console + 216u), false);
    runtime.WriteWord(console + 268u, 0u);
    runtime.WriteWord(console + 272u, 0u);
    if (runtime.ActorFromEvent(event) == 0u) return 0u;

    runtime.WriteWord(runtime.CombatSystem() + 392u,
                      runtime.ReadWord(event + 4u));
    const auto system = runtime.CombatSystem();
    runtime.WriteWord(system + 388u, runtime.ActorFromEvent(event));
    const auto manager = runtime.ActorManager();
    const auto value = runtime.ReadWord(event + 4u);
    const auto actor = runtime.ActorFromEvent(event);
    runtime.RegisterCombatAction(
        manager, runtime.CreateCombatAction(actor, value, 1u));
    return runtime.CompleteCombatResult(
        runtime.ActorFromEvent(event), runtime.ReadWord(event + 4u));
}

std::uint8_t CombatConsoleWindow::DispatchVisibleCombatResult(
    const std::uint32_t console, ICombatConsoleResultRuntime& runtime) {
    if (!runtime.IsVisible(runtime.ReadWord(console + 216u), false))
        return 0u;
    return runtime.DispatchSelectJoinAttack(
        runtime.ReadWord(console + 268u),
        runtime.ReadWord(console + 272u), 1u);
}

bool CombatConsoleWindow::OnCombatResultClick(
    const std::uint32_t console, const std::uint32_t,
    ICombatConsoleResultRuntime& runtime) {
    (void)DispatchVisibleCombatResult(console, runtime);
    return true;
}

void CombatConsoleWindow::UpdateAlpha(
    const std::uint32_t console, const double elapsed,
    ICombatConsoleAlphaRuntime& runtime) {
    const auto window = runtime.ReadWord(console + 308u);
    if (window == 0u) return;
    const auto speed = runtime.ReadFloat(console + 312u);
    const auto alpha = static_cast<double>(runtime.ReadFloat(window + 232u)) +
                       static_cast<double>(speed) * elapsed;
    auto output = static_cast<float>(alpha);
    if (speed > 0.0f && alpha > 1.0) {
        runtime.WriteFloat(console + 312u, -speed);
        output = 1.0f;
    } else if (speed <= 0.0f && alpha < 0.0) {
        runtime.WriteFloat(console + 312u, -speed);
        output = 0.0f;
    }
    runtime.SetAlpha(window, output);
}

}  // namespace pal4
