#include "pal4/combat_console.h"

#include <bit>
#include <new>

namespace pal4 {
namespace {

void QueueLegacyRefreshTimer(const std::uint32_t console,
                       ICombatConsoleDialogEventRuntime& runtime) {
    const auto callback = runtime.Allocate(16u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, 0x54cac0u);
        runtime.WriteWord(callback, 0x84553cu);
        runtime.WriteWord(callback + 4u, console);
        runtime.WriteWord(callback + 12u, 0u);
    }

    TimedQueueRecord record{};
    runtime.InitializeName(record, console + 276u);
    record.delay = std::bit_cast<std::uint32_t>(0.4f);
    record.owns_callback = callback != 0u;
    record.callback = callback;
    const auto queue = runtime.ReadWord(runtime.UiFrameManager() + 416u);
    const auto head = runtime.ReadWord(queue + 16u);
    const auto successor = runtime.ReadWord(head + 4u);
    const auto node = runtime.Allocate(36u);
    if (node == 0u) throw std::bad_alloc();
    runtime.WriteWord(node, head);
    runtime.WriteWord(node + 4u, successor != 0u ? successor : node);
    runtime.WriteWord(head + 4u, node);
    runtime.WriteWord(runtime.ReadWord(node + 4u), node);
    runtime.CopyRecord(node + 8u, record);
    runtime.WriteWord(queue + 20u,
                      runtime.ReadWord(queue + 20u) + 1u);
    if (record.owns_callback != 0u && record.callback != 0u)
        runtime.DeleteCallback(record.callback, 1u);
    if (record.name_data != 0u) runtime.ReleaseName(record);
}

}  // namespace

void ICombatConsoleDialogEventRuntime::QueueRefreshTimer(std::uint32_t console) {
    QueueLegacyRefreshTimer(console,*this);
}

std::uint32_t CombatConsoleWindow::HandleCombatDialog(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsoleActionRuntime& runtime) {
    const auto actor = runtime.ActorFromEvent(event);
    const auto type_actor = runtime.ActorFromEvent(event);
    const auto limit = runtime.ReadWord(type_actor + 804u) == 2u ? 6 : 3;
    const auto index = runtime.ActorIndex(actor);
    if (index >= limit) return static_cast<std::uint32_t>(index);

    const auto hide_index = runtime.ActorIndex(actor);
    runtime.SetVisible(runtime.ReadWord(
        console + 236u +
        4u * static_cast<std::uint32_t>(hide_index)), false);
    const auto memory = runtime.Allocate(88u);
    const auto element = memory == 0u ? 0u :
                         runtime.ConstructVisualElement(memory);
    runtime.SetElementPosition(element, 750.0f, 50.0f);
    runtime.SetElementImage(element, "zhangdouxiaotouxiang0",
                            runtime.ActorImageName(actor), 1.0f);
    runtime.SetElementFont(element, 0.4f, 0.4f);
    runtime.SetElementMotion(element, 5.0f, 0.2f, 5.0f, 0.2f);
    runtime.AddElement(element);

    const auto transformed = runtime.TransformFrameCoordinates(
        {750.0f, 110.0f});
    const auto camera = runtime.CurrentCombatCamera();
    const auto position = runtime.TransformEffectPosition(
        camera, transformed, 300.1f);
    const auto effect = runtime.CreateEffect("jdg", position);
    if (runtime.EffectNeedsUpdate(effect)) {
        const auto frame = runtime.CombatCameraFrame();
        runtime.RwFrameAddChild(
            frame, runtime.UpdateEffectProperties(effect));
    }
    runtime.QueueRefreshTimer(console);
    return runtime.ResetEffect(effect);
}

std::uint32_t CombatConsoleWindow::SetDialogTargetWindow(
    const std::uint32_t console, const std::uint32_t event,
    ICombatConsolePanelRuntime& runtime) {
    const auto actor = runtime.ActorFromEvent(event);
    const auto index = runtime.ActorIndex(actor);
    const auto type = runtime.ReadWord(actor + 804u);
    if (type == 1u && index < 3)
        runtime.WriteWord(console + 308u,
                          runtime.ReadWord(console + 224u +
                              4u * static_cast<std::uint32_t>(index)));
    else if (type != 1u && index < 6)
        runtime.WriteWord(console + 308u,
                          runtime.ReadWord(console + 236u +
                              4u * static_cast<std::uint32_t>(index)));
    return static_cast<std::uint32_t>(index);
}

void CombatConsoleWindow::ClearDialogTargetWindow(
    const std::uint32_t console, const std::uint32_t,
    ICombatConsolePanelRuntime& runtime) {
    const auto window = runtime.ReadWord(console + 308u);
    if (window != 0u) runtime.SetAlpha(window, 1.0f);
    runtime.WriteWord(console + 308u, 0u);
}

void CombatConsoleWindow::HideDialogPanel(
    const std::uint32_t console, const std::uint32_t,
    ICombatConsolePanelRuntime& runtime) {
    runtime.SetVisible(runtime.ReadWord(console + 260u), false);
}

}  // namespace pal4
