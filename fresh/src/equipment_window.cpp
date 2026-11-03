#include "pal4/equipment_window.h"

#include <string>

namespace pal4 {
namespace {

void RegisterCommand(std::uint32_t owner, std::string_view name,
                     std::uint32_t handler, IEquipmentWindowRuntime& runtime) {
    auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, owner);
        runtime.WriteWord(callback, EquipmentWindow::kCommandCallbackVtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                name, callback);
    } catch (...) {
        if (callback != 0u) runtime.DeleteCallback(callback, 1u);
        throw;
    }
    if (callback != 0u) runtime.DeleteCallback(callback, 1u);
}

void Subscribe(std::uint32_t owner, std::uint32_t window,
               std::string_view event, std::uint32_t handler,
               IEquipmentWindowRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 0x10u, owner);
        runtime.WriteWord(callback + 8u, handler);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback, EquipmentWindow::kEventCallbackVtable);
    }
    std::uint32_t reference{};
    try {
        reference = runtime.Subscribe(window, event, callback);
    } catch (...) {
        if (callback != 0u) runtime.FreeCallback(callback);
        throw;
    }
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

void InitializeControls(EquipmentWindowState& state,
                        std::uint32_t owner,
                        IEquipmentWindowRuntime& runtime) {
    struct NamedWindow { std::uint32_t offset; std::string_view name; bool by_name; };
    constexpr NamedWindow fixed[]{
        {0x40u, "EquipmentWindow/List", true},
        {0x44u, "EquipmentWindow/StaticRoleName", true},
        {0x48u, "EquipmentWindow/BtnLeftArrow", true},
        {0x4cu, "EquipmentWindow/BtnRightArrow", true},
        {0x68u, "EquipmentWindow/StaticDescription", true},
        {0x6cu, "EquipmentWindow/StaticProperty", true},
        {0xb8u, "EquipmentWindow/PanelList", false},
        {0xbcu, "EquipmentWindow/PanelDesc", false},
        {0xc0u, "EquipmentWindow/PanelProp", false},
        {0xc4u, "EquipmentWindow/ImageRoleDataPanel", false},
        {0xb0u, "EquipmentWindow/PropertyClassImage", false},
        {0xb4u, "EquipmentWindow/StaticLevel", false}};
    for (const auto& item : fixed)
        state.Write(item.offset, item.by_name ?
            runtime.FindWindowByName(item.name) : runtime.FindWindow(item.name));
    runtime.SetFont(state.Read(0xb4u), "Level_font");
    runtime.SetFormatting(state.Read(0xb4u), 2u, 2u);

    for (std::uint32_t i = 0u; i < 6u; ++i) {
        const auto button = runtime.FindWindowByName(
            "EquipmentWindow/BtnEquipmentClass" + std::to_string(i));
        state.Write(0x50u + i * 4u, button);
        runtime.WriteWord(button + 0x118u, i);
    }
    for (std::uint32_t i = 0u; i < 4u; ++i)
        state.Write(0xe0u + i * 4u, runtime.FindWindow(
            "EquipmentWindow/ImgRole" + std::to_string(i)));
    for (std::uint32_t i = 0u; i < 10u; ++i) {
        const auto control = runtime.FindWindowByName(
            "EquipmentWindow/StaticAttr" + std::to_string(i + 1u));
        state.Write(0x70u + i * 4u, control);
        runtime.WriteWord(control + 0x118u, i);
    }
    constexpr std::uint32_t equip_tags[]{0u, 2u, 3u, 1u, 4u, 5u};
    for (std::uint32_t i = 0u; i < 6u; ++i) {
        const auto image = runtime.FindWindowByName(
            "EquipmentWindow/ImageRoleEquip" + std::to_string(i + 1u));
        state.Write(0x98u + i * 4u, image);
        runtime.WriteWord(image + 0x118u, equip_tags[i]);
    }

    Subscribe(owner, state.Read(0x40u), "MouseClick", 0x452bd0u, runtime);
    Subscribe(owner, state.Read(0x40u), "HoverRowChanged", 0x452bf0u,
              runtime);
    for (std::uint32_t i = 0u; i < 6u; ++i)
        Subscribe(owner, state.Read(0x50u + i * 4u), "Clicked",
                  0x452c20u, runtime);
    constexpr std::uint32_t primary_mouse[]{0x64u, 0x50u, 0x54u,
                                             0x58u, 0x5cu, 0x60u};
    for (const auto offset : primary_mouse)
        runtime.SetupMouseEventHandlers(owner + 0x28u, state.Read(offset));
    for (std::uint32_t i = 0u; i < 6u; ++i)
        Subscribe(owner, state.Read(0x98u + i * 4u), "Clicked",
                  0x452e40u, runtime);
    Subscribe(owner, state.Read(0x48u), "Clicked", 0x452ca0u, runtime);
    Subscribe(owner, state.Read(0x4cu), "Clicked", 0x452d70u, runtime);

    const auto list = state.Read(0x40u);
    runtime.SetAlwaysOnTop(list, true);
    while (runtime.ColumnCount(list) != 0u)
        runtime.RemoveColumn(list, 0u);
    runtime.SetUserColumnSizingEnabled(list, false);
    runtime.SetUserColumnDraggingEnabled(list, false);
    runtime.SetUserSortControlEnabled(list, false);
    runtime.AddColumn(list, "", 0u, 0.60000002f);
    runtime.SetColumnLabelImage(list, 0u, "zi0", "mingcheng");
    runtime.AddColumn(list, "", 1u, 0.34999999f);
    runtime.SetColumnLabelImage(list, 1u, "zi0", "dengji");
    runtime.SetEnabled(state.Read(0x6cu), false);
    runtime.SetEnabled(state.Read(0x68u), false);

    constexpr std::string_view images[]{"wuqi", "toubu", "shenti",
                                         "zhubu", "peidai", "xinzeng"};
    for (std::uint32_t i = 0u; i < 6u; ++i)
        state.Write(0xc8u + i * 4u, runtime.GetImage("zi0", images[i]));
    state.Write(0x110u, owner);
    state.Write(0x130u, owner);
    constexpr std::uint32_t secondary_mouse[]{0x98u, 0x9cu, 0xa8u,
                                               0xa4u, 0xa0u, 0xacu};
    for (const auto offset : secondary_mouse)
        runtime.SetupMouseEventHandlers(owner + 0x118u,
                                         state.Read(offset));
    runtime.ActivateMember(owner + 0x118u);
}

}  // namespace

EquipmentWindowState& EquipmentWindow::Construct(
    EquipmentWindowState& state, std::uint32_t object_address,
    std::uint32_t name, IEquipmentWindowRuntime& runtime) {
    runtime.LoadLayout(state, name, 0u, 0u);
    try {
        runtime.ConstructMember(state, 0x28u);
    } catch (...) {
        runtime.DestroyDialogBase(state);
        throw;
    }
    state.Write(0x28u, 0x843628u);
    state.Write(0xf0u, 5u);
    for (std::uint32_t offset = 0xf4u; offset <= 0x100u; offset += 4u)
        state.Write(offset, 0u);
    try {
        runtime.ConstructNameNode(state);
    } catch (...) {
        runtime.CleanupMember(state, 0x28u);
        runtime.DestroyDialogBase(state);
        throw;
    }
    state.Write(0x10cu, kNameNodeVtable);
    try {
        runtime.ConstructMember(state, 0x118u);
    } catch (...) {
        runtime.ResetNameNode(state);
        runtime.CleanupMember(state, 0x28u);
        runtime.DestroyDialogBase(state);
        throw;
    }
    state.Write(0x118u, kSecondaryMemberVtable);
    state.Write(0x28u, kPrimaryMemberVtable);
    state.Write(0u, kObjectVtable);
    try {
        RegisterCommand(object_address, "use_equipment", 0x4502b0u,
                        runtime);
        RegisterCommand(object_address, "discard_equipment", 0x450370u,
                        runtime);
        InitializeControls(state, object_address, runtime);
    } catch (...) {
        runtime.CleanupMember(state, 0x118u);
        runtime.ResetNameNode(state);
        runtime.CleanupMember(state, 0x28u);
        runtime.DestroyDialogBase(state);
        throw;
    }
    return state;
}

std::uint32_t EquipmentWindow::Destroy(
    EquipmentWindowState& state, std::uint32_t object_address,
    IEquipmentWindowRuntime& runtime) {
    runtime.FreeBuffer(state.Read(0x120u));
    state.Write(0x120u, 0u);
    state.Write(0x124u, 0u);
    state.Write(0x128u, 0u);
    runtime.ResetNameNode(state);
    runtime.FreeBuffer(state.Read(0x30u));
    state.Write(0x30u, 0u);
    state.Write(0x34u, 0u);
    state.Write(0x38u, 0u);
    runtime.DestroyDialogBase(state);
    return object_address;
}

}  // namespace pal4
