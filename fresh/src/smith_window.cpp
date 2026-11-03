#include "pal4/smith_window.h"

#include <array>
#include <cstddef>

namespace pal4 {
namespace {

struct WindowBinding {
    std::uint32_t offset;
    std::string_view name;
};

constexpr std::array<WindowBinding, 22> kWindows{{
    {0x60u, "SmithWindow/ButtonFound"},
    {0x64u, "SmithWindow/ButtonSmithery"},
    {0x68u, "SmithWindow/ButtonAddMagic"},
    {0x40u, "SmithWindow/ListEquipment"},
    {0x48u, "SmithWindow/EditEquipmentInfo"},
    {0x78u, "SmithWindow/zuozi"},
    {0x4cu, "SmithWindow/EditEquipmentAttr"},
    {0x58u, "SmithWindow/ButtonLeft"},
    {0x5cu, "SmithWindow/ButtonRight"},
    {0x44u, "SmithWindow/ListPrescription"},
    {0x50u, "SmithWindow/EditPrescriptionInfo"},
    {0x54u, "SmithWindow/EditPrescriptionAttr"},
    {0x6cu, "SmithWindow/StaticEquipmentCaption"},
    {0x70u, "SmithWindow/StaticPrescriptionCaption"},
    {0x74u, "SmithWindow/StaticImageSystemBar"},
    {0x7cu, "SmithWindow/ImgDisusedMask"},
    {0x90u, "SmithWindow/WindowEquipment"},
    {0x94u, "SmithWindow/WindowEquipmentInfo"},
    {0x98u, "SmithWindow/WindowEquipmentAttr"},
    {0x84u, "SmithWindow/WindowPrescription"},
    {0x88u, "SmithWindow/WindowPrescriptionInfo"},
    {0x8cu, "SmithWindow/WindowPrescriptionAttr"},
}};

struct EventBinding {
    std::uint32_t window_offset;
    SmithWindowEvent event;
    std::uint32_t handler;
};

constexpr std::array<EventBinding, 9> kEvents{{
    {0x60u, SmithWindowEvent::Clicked, 0x49e200u},
    {0x64u, SmithWindowEvent::Clicked, 0x49e360u},
    {0x68u, SmithWindowEvent::Clicked, 0x49e4c0u},
    {0x58u, SmithWindowEvent::Clicked, 0x49dd00u},
    {0x5cu, SmithWindowEvent::Clicked, 0x49dd80u},
    {0x40u, SmithWindowEvent::HoverRowChanged, 0x49c460u},
    {0x40u, SmithWindowEvent::MouseClick, 0x49c520u},
    {0x44u, SmithWindowEvent::HoverRowChanged, 0x49c4a0u},
    {0x44u, SmithWindowEvent::MouseClick, 0x49d5c0u},
}};

void BindWindow(const std::uint32_t object,
                const WindowBinding& binding,
                ISmithWindowRuntime& runtime) {
    runtime.WriteWord(object + binding.offset,
                      runtime.GetWindowByName(object, binding.name));
}

std::uint32_t Window(const std::uint32_t object,
                     const std::uint32_t offset,
                     ISmithWindowRuntime& runtime) {
    return runtime.ReadWord(object + offset);
}

void MakeReadOnly(const std::uint32_t object,
                  const std::uint32_t offset,
                  ISmithWindowRuntime& runtime) {
    const auto window = Window(object, offset, runtime);
    runtime.SetReadOnly(window, true);
    runtime.SetEnabled(window, false);
}

void Subscribe(const std::uint32_t object,
               const EventBinding& binding,
               ISmithWindowRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, binding.handler);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, object);
        runtime.WriteWord(callback, 0x843d94u);
    }
    const auto subscription = runtime.Subscribe(
        Window(object, binding.window_offset, runtime),
        binding.event, callback);
    if (subscription != 0u) runtime.ReleaseSubscription(subscription);
}

}  // namespace

std::uint32_t SmithWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    ISmithWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    runtime.InitializeListAdapter(object + 0x28u);
    runtime.WriteWord(object + 0x28u, 0x843628u);
    runtime.WriteByte(object + 0x80u, 0u);
    runtime.WriteByte(object + 0x81u, 0u);
    runtime.WriteWord(object + 0x9cu, 0u);
    runtime.WriteWord(object + 0xa0u, 0u);
    runtime.InitializeNameNode(object + 0xa4u);
    const auto flag = static_cast<std::uint8_t>(layout_name);
    for (const auto offset : {0xb8u, 0xc8u}) {
        runtime.WriteByte(object + offset, flag);
        for (const auto tail : {4u, 8u, 12u})
            runtime.WriteWord(object + offset + tail, 0u);
    }
    runtime.WriteByte(object + 0xd8u, flag);
    runtime.ResetTextFlag(object + 0xd8u, 0u);
    runtime.WriteWord(object + 0xe8u, 0x843df4u);
    runtime.WriteWord(object + 0xf0u, 0x843ddcu);
    runtime.WriteWord(object, 0x843dc4u);
    runtime.WriteWord(object + 0x28u, 0x843d9cu);
    runtime.WriteByte(object + 0xacu, 0u);

    for (std::size_t i = 0u; i < 6u; ++i)
        BindWindow(object, kWindows[i], runtime);
    runtime.SetAlwaysOnTop(Window(object, 0x78u, runtime), true);
    MakeReadOnly(object, 0x48u, runtime);
    BindWindow(object, kWindows[6], runtime);
    MakeReadOnly(object, 0x4cu, runtime);
    for (std::size_t i = 7u; i < 11u; ++i)
        BindWindow(object, kWindows[i], runtime);
    MakeReadOnly(object, 0x50u, runtime);
    BindWindow(object, kWindows[11], runtime);
    MakeReadOnly(object, 0x54u, runtime);
    for (std::size_t i = 12u; i < kWindows.size(); ++i)
        BindWindow(object, kWindows[i], runtime);
    for (const auto offset : {0x90u, 0x94u, 0x98u,
                              0x84u, 0x88u, 0x8cu})
        runtime.SetVisible(Window(object, offset, runtime), false);

    for (const auto& binding : kEvents)
        Subscribe(object, binding, runtime);
    for (const auto offset : {0x40u, 0x44u})
        runtime.SetShowVertScrollbar(Window(object, offset, runtime), false);
    runtime.ResetState(object);
    for (const auto offset : {0x60u, 0x64u, 0x68u})
        runtime.SetupMouseEventHandlers(object + 0x28u,
                                        Window(object, offset, runtime));
    runtime.WriteWord(object + 0xecu, object);
    runtime.WriteWord(object + 0xf4u, object);
    runtime.SetAlwaysOnTop(Window(object, 0x58u, runtime), true);
    runtime.SetAlwaysOnTop(Window(object, 0x5cu, runtime), true);
    runtime.SetAlwaysOnTop(Window(object, 0x6cu, runtime), false);
    runtime.SetEnabled(Window(object, 0x6cu, runtime), false);
    runtime.SetAlwaysOnTop(Window(object, 0x24u, runtime), true);
    return object;
}

std::uint32_t SmithWindow::Destruct(
    const std::uint32_t object,
    ISmithWindowDestructionRuntime& runtime) {
    const auto shared_text = runtime.ReadWord(object + 0xdcu);
    if (shared_text != 0u) {
        const auto header = shared_text - 1u;
        const auto reference = runtime.ReadByte(header);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    for (const auto offset : {0xdcu, 0xe0u, 0xe4u})
        runtime.WriteWord(object + offset, 0u);

    for (const auto offset : {0xccu, 0xbcu}) {
        runtime.Free(runtime.ReadWord(object + offset));
        for (const auto tail : {0u, 4u, 8u})
            runtime.WriteWord(object + offset + tail, 0u);
    }
    runtime.ResetEffectHandle(object + 0xa4u);
    const auto adapter = object == 0u ? 0u : object + 0x28u;
    runtime.Free(runtime.ReadWord(adapter + 8u));
    for (const auto tail : {8u, 12u, 16u})
        runtime.WriteWord(adapter + tail, 0u);
    return runtime.DestructBase(object);
}

}  // namespace pal4
