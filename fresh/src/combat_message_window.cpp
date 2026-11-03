#include "pal4/combat_message_window.h"

namespace pal4 {
namespace {

void RemoveMessageNode(const std::uint32_t node,
                       ICombatMessageWindowRuntime& runtime) {
    const auto next = runtime.ReadWord(node);
    const auto previous = runtime.ReadWord(node + 4u);
    runtime.WriteWord(previous, next);
    runtime.WriteWord(next + 4u, previous);
    (void)CombatMessageStringRecord::Cleanup(node + 8u, 0u, runtime);
    runtime.Free(node);
}

std::u32string DecodeRecord(const std::uint32_t record,
                            ICombatMessageWindowRuntime& runtime) {
    const auto text = runtime.ReadCString(runtime.ReadWord(record));
    return runtime.DecodeUtf8(runtime.ConvertToUtf8(text));
}

}  // namespace

std::uint32_t CombatMessageStringRecord::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    ICombatMessageWindowRuntime& runtime) {
    if (destination) runtime.CopyMessageString(destination, source);
    return destination;
}

std::uint32_t CombatMessageStringRecord::Cleanup(
    const std::uint32_t record, const std::uint32_t flags,
    ICombatMessageWindowRuntime& runtime) {
    runtime.CleanupMessageString(record);
    if ((flags & 1u) != 0u) runtime.Free(record);
    return record;
}

std::uint32_t CombatMessageWindow::Construct(
    const std::uint32_t message, const std::uint32_t,
    ICombatMessageWindowRuntime& runtime) {
    runtime.LoadLayout(message, "CombatMessageWindow",
                       "CombatMessageFrame", "CombatMessageFrame");
    runtime.WriteWord(message + 40u, 0u);
    runtime.WriteWord(message + 44u, 0u);
    runtime.WriteByte(message + 48u, 0u);
    const auto head = runtime.Allocate(20u);
    if (!head) {
        (void)runtime.DestructLayout(message);
        return 0u;
    }
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 4u, head);
    runtime.WriteWord(message + 52u, head);
    runtime.WriteWord(message + 56u, 0u);
    runtime.WriteWord(message, kVtable);
    (void)Initialize(message, runtime);
    runtime.WriteWord(message + 60u, 100u);
    return message;
}

std::uint32_t CombatMessageWindow::Destruct(
    const std::uint32_t message, ICombatMessageWindowRuntime& runtime) {
    (void)ClearMessages(message, runtime);
    return runtime.DestructLayout(message);
}

std::uint32_t CombatMessageWindow::ClearMessages(
    const std::uint32_t message, ICombatMessageWindowRuntime& runtime) {
    const auto head = runtime.ReadWord(message + 52u);
    if (head) {
        for (auto node = runtime.ReadWord(head); node != head;) {
            const auto next = runtime.ReadWord(node);
            RemoveMessageNode(node, runtime);
            runtime.WriteWord(message + 56u,
                              runtime.ReadWord(message + 56u) - 1u);
            node = next;
        }
        runtime.Free(head);
    }
    runtime.WriteWord(message + 52u, 0u);
    runtime.WriteWord(message + 56u, 0u);
    return 0u;
}

std::uint32_t CombatMessageWindow::SetVisible(
    const std::uint32_t message, const bool visible,
    const std::uint32_t parent, ICombatMessageWindowRuntime& runtime) {
    runtime.SetParent(message, visible, parent);
    return visible ? CombatMessageSubscriptions::Register(message, runtime)
                   : CombatMessageSubscriptions::Clear(message, runtime);
}

std::uint32_t CombatMessageWindow::Initialize(
    const std::uint32_t message, ICombatMessageWindowRuntime& runtime) {
    const auto prefix_pointer = runtime.ReadWord(message + 24u);
    const auto prefix = prefix_pointer ? runtime.ReadCString(prefix_pointer)
                                       : std::string{};
    const auto name = runtime.ConvertToUtf8(prefix + "EditRoleProperty");
    const auto edit = runtime.GetWindowByName(message, name);
    runtime.WriteWord(message + 64u, edit);
    runtime.SetNormalTextColour(edit, 0.0f, 1.0f, 0.0f, 1.0f);
    runtime.SetText(edit, "");
    return runtime.CleanupTemporaryString();
}

std::uint32_t CombatMessageWindow::InsertMessageRecord(
    const std::uint32_t message, const std::uint32_t source_record,
    ICombatMessageWindowRuntime& runtime) {
    const auto head = runtime.ReadWord(message + 52u);
    if (!head) return 0u;
    const auto node = runtime.Allocate(20u);
    if (!node) return 0u;
    const auto previous = runtime.ReadWord(head + 4u);
    runtime.WriteWord(node, head);
    runtime.WriteWord(node + 4u, previous);
    runtime.WriteWord(head + 4u, node);
    runtime.WriteWord(previous, node);
    (void)CombatMessageStringRecord::CopyConstruct(
        node + 8u, source_record, runtime);
    runtime.WriteWord(message + 56u,
                      runtime.ReadWord(message + 56u) + 1u);
    return node;
}

void CombatMessageWindow::AddMessage(
    const std::uint32_t message, const std::uint32_t source_record,
    ICombatMessageWindowRuntime& runtime) {
    const auto edit = runtime.ReadWord(message + 64u);
    const auto count = runtime.ReadWord(message + 56u);
    const auto limit = runtime.ReadWord(message + 60u);
    if (count <= limit) {
        if (!InsertMessageRecord(message, source_record, runtime)) return;
        auto text = runtime.ReadEditText(edit);
        text += DecodeRecord(source_record, runtime);
        text += U'\n';
        runtime.SetUnicodeText(edit, text);
        runtime.SetCaretIndex(edit,
                              static_cast<std::uint32_t>(text.size() - 1u));
        return;
    }

    const auto head = runtime.ReadWord(message + 52u);
    const auto oldest = runtime.ReadWord(head);
    if (oldest != head) {
        RemoveMessageNode(oldest, runtime);
        runtime.WriteWord(message + 56u,
                          runtime.ReadWord(message + 56u) - 1u);
    }
    if (!InsertMessageRecord(message, source_record, runtime)) return;
    std::u32string text;
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        text += DecodeRecord(node + 8u, runtime);
        text += U'\n';
    }
    runtime.SetUnicodeText(edit, text);
    runtime.SetCaretIndex(edit,
                          static_cast<std::uint32_t>(text.size() - 1u));
}

bool CombatMessageWindow::Show(
    const std::uint32_t event, ICombatMessageWindowRuntime& runtime) {
    const auto visible = runtime.ReadWord(event) != 0u;
    const auto parent = runtime.ReadWord(event + 4u);
    const auto type = runtime.ReadWord(0x8b6920u);
    auto frame = runtime.FindFrame(runtime.FrameManager(), type);
    if (visible && !frame) {
        runtime.RegisterFrame(runtime.FrameManager(), type);
        frame = runtime.FindFrame(runtime.FrameManager(), type);
    }
    if (!frame) return false;
    runtime.SetFrameVisible(frame, visible, parent);
    return true;
}

void CombatMessageWindow::HandleEvent(
    const std::uint32_t message, const std::uint32_t event,
    ICombatMessageWindowRuntime& runtime) {
    AddMessage(message, runtime.ReadWord(event), runtime);
}

}  // namespace pal4
