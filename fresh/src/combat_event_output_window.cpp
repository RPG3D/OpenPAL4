#include "pal4/combat_event_output_window.h"

#include <array>

namespace pal4 {
namespace {

void UnlinkAndFree(const std::uint32_t node,
                   ICombatEventOutputRuntime& runtime) {
    const auto next = runtime.ReadWord(node);
    const auto previous = runtime.ReadWord(node + 4u);
    runtime.WriteWord(previous, next);
    runtime.WriteWord(next + 4u, previous);
    runtime.CleanupString(node + 8u);
    runtime.Free(node);
}

void AddEvent(const std::uint32_t window, const std::uint32_t event,
              ICombatEventOutputRuntime& runtime) {
    const auto selected = runtime.GetSelectedComboItem(
        runtime.ReadWord(window + 68u));
    if (!selected) return;
    const auto level = static_cast<std::int32_t>(runtime.ReadWord(event + 4u));
    const auto threshold = static_cast<std::int32_t>(
        runtime.ReadWord(selected + 312u));
    if (level > threshold) return;

    const auto head = runtime.ReadWord(window + 52u);
    const auto node = runtime.Allocate(20u);
    if (!node) return;
    const auto previous = runtime.ReadWord(head + 4u);
    runtime.WriteWord(node, head);
    runtime.WriteWord(node + 4u, previous);
    runtime.WriteWord(head + 4u, node);
    runtime.WriteWord(previous, node);
    runtime.CopyString(node + 8u,
        runtime.ReadCString(runtime.ReadWord(event)));
    runtime.WriteWord(window + 56u,
        runtime.ReadWord(window + 56u) + 1u);
    CombatEventOutputWindow::RebuildText(window, runtime);
}

}  // namespace

std::uint32_t CombatEventOutputWindow::Construct(
    const std::uint32_t window, const std::uint32_t layout,
    ICombatEventOutputRuntime& runtime) {
    runtime.LoadLayout(window, layout);
    runtime.WriteWord(window + 40u, 0u);
    runtime.WriteWord(window + 44u, 0u);
    runtime.WriteByte(window + 48u,
                      static_cast<std::uint8_t>(layout));
    const auto head = runtime.Allocate(20u);
    if (!head) return 0u;
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 4u, head);
    runtime.WriteWord(window + 52u, head);
    runtime.WriteWord(window + 56u, 0u);
    runtime.WriteWord(window + 60u, 20u);
    runtime.WriteWord(window, kVtable);
    (void)Initialize(window, runtime);
    return window;
}

std::uint32_t CombatEventOutputWindow::DeletingDestructor(
    const std::uint32_t window, const std::uint8_t flags,
    ICombatEventOutputRuntime& runtime) {
    (void)Destruct(window, runtime);
    if ((flags & 1u) != 0u) runtime.Free(window);
    return window;
}

std::uint32_t CombatEventOutputWindow::Destruct(
    const std::uint32_t window, ICombatEventOutputRuntime& runtime) {
    const auto head = runtime.ReadWord(window + 52u);
    if (head) {
        for (auto node = runtime.ReadWord(head); node != head;) {
            const auto next = runtime.ReadWord(node);
            UnlinkAndFree(node, runtime);
            runtime.WriteWord(window + 56u,
                runtime.ReadWord(window + 56u) - 1u);
            node = next;
        }
        runtime.Free(head);
    }
    runtime.WriteWord(window + 52u, 0u);
    runtime.WriteWord(window + 56u, 0u);
    return runtime.DestructLayout(window);
}

std::uint32_t CombatEventOutputWindow::SetVisible(
    const std::uint32_t window, const bool visible,
    const std::uint32_t parent, ICombatEventOutputRuntime& runtime) {
    runtime.SetParent(window, visible, parent);
    return visible ? RegisterEvents(window, runtime)
                   : ClearEvents(window, runtime);
}

std::uint32_t CombatEventOutputWindow::Initialize(
    const std::uint32_t window, ICombatEventOutputRuntime& runtime) {
    runtime.WriteWord(window + 64u,
        runtime.FindControl(window, "CombatEventOutput/EditEventOutput"));
    const auto combo = runtime.FindControl(
        window, "CombatEventOutput/ComboEventLv");
    runtime.WriteWord(window + 68u, combo);
    runtime.WriteWord(window + 72u,
        runtime.FindControl(window,
                            "CombatEventOutput/WinCombatEventOutput"));
    constexpr std::array<std::string_view, 4> labels{{
        "CEL_PRIOR", "CEL_STANDARD", "CEL_INFORMATIVE", "CEL_INSANE",
    }};
    for (std::uint32_t index = 0u; index < labels.size(); ++index) {
        const auto item = runtime.Allocate(0x26cu);
        runtime.ConstructLevelItem(item, labels[index]);
        runtime.WriteWord(item + 312u, index);
        runtime.AddComboItem(combo, item);
    }
    runtime.SelectComboItem(combo, 0u, true);
    const auto callback = runtime.Allocate(12u);
    if (callback) {
        runtime.WriteWord(callback, kComboCallbackVtable);
        runtime.WriteWord(callback + 4u, kComboCallback);
        runtime.WriteWord(callback + 8u, window);
    }
    const auto subscription = runtime.SubscribeComboSelection(combo, callback);
    if (subscription) runtime.ReleaseSubscription(subscription);
    return runtime.CleanupTemporaryString();
}

std::uint32_t CombatEventOutputWindow::RegisterEvents(
    const std::uint32_t window, ICombatEventOutputRuntime& runtime) {
    const auto first = runtime.RegisterRouterEvent(runtime.EventRouter(), 2u,
        runtime.ReadWord(kHandledKeyPointer), window, kHandledCallback);
    runtime.WriteWord(window + 40u, first);
    const auto second = runtime.RegisterRouterEvent(runtime.EventRouter(), 2u,
        runtime.ReadWord(kUnhandledKeyPointer), window, kUnhandledCallback);
    // The second store overwrites the first handle in the original code.
    runtime.WriteWord(window + 40u, second);
    return second;
}

std::uint32_t CombatEventOutputWindow::ClearEvents(
    const std::uint32_t window, ICombatEventOutputRuntime& runtime) {
    (void)runtime.RemoveRouterEvent(runtime.EventRouter(),
        runtime.ReadWord(kHandledKeyPointer), runtime.ReadWord(window + 40u));
    runtime.WriteWord(window + 40u, 0u);
    const auto result = runtime.RemoveRouterEvent(runtime.EventRouter(),
        runtime.ReadWord(kUnhandledKeyPointer), runtime.ReadWord(window + 44u));
    runtime.WriteWord(window + 44u, 0u);
    return result;
}

void CombatEventOutputWindow::HandleEventHandled(
    const std::uint32_t window, const std::uint32_t event,
    ICombatEventOutputRuntime& runtime) {
    AddEvent(window, event, runtime);
}

void CombatEventOutputWindow::HandleEventUnhandled(
    const std::uint32_t window, const std::uint32_t event,
    ICombatEventOutputRuntime& runtime) {
    AddEvent(window, event, runtime);
}

void CombatEventOutputWindow::RebuildText(
    const std::uint32_t window, ICombatEventOutputRuntime& runtime) {
    const auto head = runtime.ReadWord(window + 52u);
    if (runtime.ReadWord(window + 56u) >
        runtime.ReadWord(window + 60u)) {
        const auto oldest = runtime.ReadWord(head);
        UnlinkAndFree(oldest, runtime);
        runtime.WriteWord(window + 56u,
            runtime.ReadWord(window + 56u) - 1u);
    }
    std::u32string text;
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        const auto value = runtime.ReadCString(runtime.ReadWord(node + 8u));
        for (const auto byte : value)
            text.push_back(static_cast<unsigned char>(byte));
        text.push_back(U'\n');
    }
    runtime.SetText(runtime.ReadWord(window + 64u), text);
}

std::uint8_t CombatEventOutputWindow::ClearEntries(
    const std::uint32_t window, ICombatEventOutputRuntime& runtime) {
    const auto head = runtime.ReadWord(window + 52u);
    for (auto node = runtime.ReadWord(head); node != head;) {
        const auto next = runtime.ReadWord(node);
        UnlinkAndFree(node, runtime);
        runtime.WriteWord(window + 56u,
            runtime.ReadWord(window + 56u) - 1u);
        node = next;
    }
    RebuildText(window, runtime);
    return 1u;
}

std::uint32_t CombatEventOutputFactory::GetTypeThunk(
    ICombatEventOutputRuntime& runtime) {
    return GetType(runtime);
}

std::uint32_t CombatEventOutputFactory::GetType(
    ICombatEventOutputRuntime& runtime) {
    const auto type = runtime.ReadWord(kTypePointer);
    runtime.WriteWord(kTypeCache, type);
    return type;
}

std::uint32_t CombatEventOutputFactory::Create(
    const std::uint32_t layout, ICombatEventOutputRuntime& runtime) {
    const auto window = runtime.Allocate(0x4cu);
    return window ? CombatEventOutputWindow::Construct(window, layout, runtime)
                  : 0u;
}

std::uint32_t CombatEventOutputFactory::RegisterFactoryThunk(
    ICombatEventOutputRuntime& runtime) {
    return RegisterFactory(runtime);
}

std::uint32_t CombatEventOutputFactory::RegisterFactory(
    ICombatEventOutputRuntime& runtime) {
    (void)runtime.FrameManager();
    return runtime.RegisterFactory(runtime.ReadWord(kTypeCache),
                                   kCreateCallback);
}

std::uint32_t CombatEventOutputFactory::RegisterShowEventThunk(
    ICombatEventOutputRuntime& runtime) {
    return RegisterShowEvent(runtime);
}

std::uint32_t CombatEventOutputFactory::RegisterShowEvent(
    ICombatEventOutputRuntime& runtime) {
    const auto handler = runtime.Allocate(8u);
    if (handler) {
        runtime.WriteWord(handler + 4u, runtime.ReadWord(kTypePointer));
        runtime.WriteWord(handler, 0x843494u);
    }
    runtime.InsertShowEvent(runtime.FrameManager() + 260u,
        kShowEventKey, handler);
    if (handler) runtime.DeleteHandler(handler, 1u);
    return kShowEventSentinel;
}

}  // namespace pal4
