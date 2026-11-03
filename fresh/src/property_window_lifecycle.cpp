#include "pal4/property_window_lifecycle.h"

namespace pal4 {
namespace {

struct Binding {
    std::string_view path;
    std::uint32_t offset;
};

constexpr std::array<Binding, 8> kBindings{{
    {"PropertyWindow/PropertyList", 88u},
    {"PropertyWindow/PropertyText", 92u},
    {"PropertyWindow/PropertyDescription", 96u},
    {"PropertyWindow/PropertyProp", 100u},
    {"PropertyWindow/PropertyClassImage", 104u},
    {"PropertyWindow/PropertyWindow", 140u},
    {"PropertyWindow/PropertyWindow2", 144u},
    {"PropertyWindow/PropertyWindow3", 148u},
}};

struct Command {
    std::string_view name;
    std::uint32_t handler;
};

constexpr std::array<Command, 3> kCommands{{
    {"use_property", 0x472f20u},
    {"discard_property", 0x473110u},
    {"after_use_property", 0x473320u},
}};

struct Button {
    std::string_view path;
    std::uint32_t offset;
    std::uint32_t tag;
};

constexpr std::array<Button, 6> kButtons{{
    {"PropertyWindow/NewProp", 84u, 5u},
    {"PropertyWindow/PropCure", 64u, 0u},
    {"PropertyWindow/Attack", 68u, 1u},
    {"PropertyWindow/Auxiliary", 72u, 2u},
    {"PropertyWindow/PropMaterial", 76u, 3u},
    {"PropertyWindow/PropScenario", 80u, 4u},
}};

constexpr std::array<std::string_view, 6> kImageNames{{
    "zi", "zi1", "zi2", "zi3", "zi4", "xinzeng",
}};

}  // namespace

PropertyWindowState& PropertyWindowLifecycle::Construct(
    PropertyWindowState& state, std::uint32_t name,
    IPropertyWindowLifecycleRuntime& runtime) {
    runtime.LoadLayout(state, name);
    auto stage = PropertyWindowInitStage::Layout;
    try {
        runtime.InitializeMember(state);
        stage = PropertyWindowInitStage::Member;
        state.Write(40u, kInitialMemberVtable);
        state.Write(116u, 0xffffffffu);
        state.Write(120u, 0u);
        state.Write(124u, 0u);
        state.Write(128u, 0u);
        runtime.InitializeNameNode(state);
        stage = PropertyWindowInitStage::NameNode;
        runtime.InitializeUiObject(state);
        stage = PropertyWindowInitStage::UiObject;
        state.Write(40u, kMemberVtable);
        state.Write(0u, kObjectVtable);

        const auto owner = runtime.ObjectAddress(state);
        for (const auto& command : kCommands) {
            runtime.RegisterCommand(command.name, command.handler,
                                    owner, kCommandVtable);
        }
        for (const auto& binding : kBindings) {
            state.Write(binding.offset, runtime.FindWindow(binding.path));
        }

        const auto list = state.Read(88u);
        runtime.Subscribe(list, PropertyWindowEvent::MouseClick,
                          0x472920u, owner, kEventVtable);
        runtime.Subscribe(list, PropertyWindowEvent::HoverRowChanged,
                          0x472f00u, owner, kEventVtable);
        for (const auto& button : kButtons) {
            const auto window = runtime.FindWindow(button.path);
            state.Write(button.offset, window);
            runtime.Subscribe(window, PropertyWindowEvent::Clicked,
                              0x4728b0u, owner, kEventVtable);
            runtime.SetButtonTag(window, button.tag);
        }

        runtime.SetVisible(runtime.FindWindow(
                               "PropertyWindow/PropertyLeftArrowButton"),
                           false);
        runtime.SetVisible(runtime.FindWindow(
                               "PropertyWindow/PropertyRightArrowButton"),
                           false);
        runtime.SetEnabled(state.Read(96u), false);
        runtime.SetEnabled(state.Read(100u), false);

        while (runtime.ColumnCount(list) != 0u) {
            runtime.RemoveColumn(list, 0u);
        }
        runtime.AddColumn(list, "", 0u, 0.5f);
        runtime.AddColumn(list, "", 1u, 0.40000001f);
        runtime.SetColumnLabelImage(list, 0u, "zi0", "mingcheng");
        runtime.SetColumnLabelImage(list, 1u, "zi0", "shuliang");
        runtime.ConfigureList(list);

        for (std::size_t i = 0; i < kImageNames.size(); ++i) {
            state.Write(static_cast<std::uint32_t>(152u + 4u * i),
                        runtime.Image("zi0", kImageNames[i]));
        }
        runtime.SetupMouseHandlers(state, state.Read(84u));
        for (const auto& button : kButtons) {
            if (button.offset != 84u) {
                runtime.SetupMouseHandlers(state, state.Read(button.offset));
            }
        }
        state.Write(180u, owner);
    } catch (...) {
        runtime.Cleanup(state, stage);
        throw;
    }
    return state;
}

std::uint32_t PropertyWindowLifecycle::Destroy(
    PropertyWindowState& state,
    IPropertyWindowLifecycleRuntime& runtime) {
    runtime.ResetNameNode(state);
    runtime.FreeMemberBuffer(state.Read(48u));
    state.Write(48u, 0u);
    state.Write(52u, 0u);
    state.Write(56u, 0u);
    return runtime.DestroyDialogBase(state);
}

}  // namespace pal4
