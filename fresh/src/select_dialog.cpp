#include "pal4/select_dialog.h"

#include <array>
#include <cstdint>
#include <string>

namespace pal4 {

namespace {

void ReleaseOwner(const std::uint32_t owner,
                  ISelectDialogRuntime& runtime) {
    if (owner == 0u) return;
    const auto count = runtime.ReadWord(owner + 4u) - 1u;
    runtime.WriteWord(owner + 4u, count);
    if (count == 0u) runtime.DestroyOwner(owner, 1u);
}

void Subscribe(const std::uint32_t object,
               const std::uint32_t window,
               const char* event,
               const std::uint32_t function,
               ISelectDialogRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, function);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, object);
        runtime.WriteWord(callback, SelectDialog::kCallbackVtable);
    }
    ReleaseOwner(runtime.Subscribe(window, event, callback),
                 runtime);
}

}  // namespace

std::uint32_t SelectDialog::Construct(
    const std::uint32_t object,
    ISelectDialogRuntime& runtime) {
    unsigned stage = 0u;
    try {
        runtime.LoadLayout(object, "SelectDialog");
        stage = 1u;
        runtime.ConstructNavigation(object + 40u);
        stage = 2u;
        runtime.WriteWord(object + 40u, kNavigationBaseVtable);
        const auto byte = runtime.IndeterminateListByte();
        runtime.WriteWord(object + 64u, 0u);
        runtime.WriteByte(object + 68u, byte);
        runtime.WriteWord(object + 72u, 0u);
        runtime.WriteWord(object + 76u, 0u);
        runtime.WriteWord(object + 80u, 0u);
        stage = 3u;
        runtime.WriteByte(object + 84u, byte);
        runtime.WriteWord(object + 88u, 0u);
        runtime.WriteWord(object + 92u, 0u);
        runtime.WriteWord(object + 96u, 0u);
        stage = 4u;
        runtime.WriteByte(object + 100u, byte);
        runtime.WriteWord(object + 104u, 0u);
        runtime.WriteWord(object + 108u, 0u);
        runtime.WriteWord(object + 112u, 0u);
        stage = 5u;
        runtime.WriteWord(object + 40u, kNavigationVtable);
        runtime.WriteWord(object + 160u, 0u);
        runtime.WriteWord(object, kVtable);

        const auto prefix = runtime.ReadLayoutPrefix(object);
        constexpr std::array<const char*, 5> suffixes{
            "EditQuestion", "Panelhead", "Panelbody",
            "Panelfoot", "StaticAnswer"};
        constexpr std::array<std::uint32_t, 5> offsets{
            132u, 136u, 140u, 144u, 148u};
        for (std::size_t index = 0u; index < suffixes.size(); ++index) {
            runtime.WriteWord(object + offsets[index],
                              runtime.FindWindow(
                                  object, prefix + suffixes[index]));
        }

        Subscribe(object, runtime.ReadWord(object + 36u),
                  "EventKeyDown", kKeyCallback, runtime);
        Subscribe(object, runtime.ReadWord(object + 148u),
                  "EventClicked", kClickCallback, runtime);

        const auto body = runtime.ReadWord(object + 140u);
        const auto body_position = runtime.GetPosition(body);
        runtime.WriteWord(object + 116u, body_position.x);
        runtime.WriteWord(object + 120u, body_position.y);
        const auto answer_position = runtime.GetPosition(
            runtime.ReadWord(object + 148u));
        runtime.WriteWord(object + 124u, answer_position.x);
        runtime.WriteWord(object + 128u, answer_position.y);
        runtime.ResetAnswerCollection(object + 84u,
                                      runtime.ReadWord(object + 92u),
                                      1u, object + 140u);
        const auto parent = runtime.ReadWord(body + 68u);
        runtime.WriteWord(object + 152u, parent);
        runtime.RemoveChild(parent, body);
        runtime.WriteWord(object + 156u, 0xffffffffu);
        return object;
    } catch (...) {
        if (stage >= 5u)
            runtime.CleanupMemoryBlock(object + 100u);
        if (stage >= 4u)
            runtime.CleanupMemoryBlock(object + 84u);
        if (stage >= 3u)
            runtime.CleanupMemoryBlock(object + 68u);
        if (stage >= 2u)
            runtime.CleanupNavigation(object + 40u);
        if (stage >= 1u) runtime.CleanupBase(object);
        throw;
    }
}

}  // namespace pal4
