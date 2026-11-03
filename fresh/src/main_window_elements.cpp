#include "pal4/main_window_elements.h"

#include <array>
#include <string>

namespace pal4 {

namespace {

struct ButtonDefinition {
    std::string_view name;
    std::uint32_t offset;
    std::uint32_t handler;
};

constexpr std::array<ButtonDefinition, 8> kButtons{{
    {"BtnNewGame", 0x40u, 0x448930u},
    {"BtnLoadGame", 0x44u, 0x448ac0u},
    {"BtnExit", 0x48u, 0x448a70u},
    {"BtnMovie", 0x4cu, 0x448970u},
    {"BtnInfomation", 0x50u, 0x448a30u},
    {"BtnAboutPal", 0x54u, 0x4489f0u},
    {"BtnQuest", 0x58u, 0x448b00u},
    {"BtnPicture", 0x5cu, 0x4489b0u},
}};

std::uint32_t FindChild(const MainWindowState& state,
                        std::string_view suffix,
                        IMainWindowElementsRuntime& runtime) {
    std::string name(runtime.LayoutPrefix(state));
    name.append(suffix);
    return runtime.FindWindow(name);
}

void ReleaseReference(std::uint32_t reference,
                      IMainWindowElementsRuntime& runtime) {
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

void Subscribe(std::uint32_t window, std::uint32_t owner,
               std::uint32_t handler, bool use_constructor,
               IMainWindowElementsRuntime& runtime) {
    const auto callback = runtime.AllocateCallback(0x18u);
    if (callback != 0u) {
        if (use_constructor) {
            runtime.ConstructCallback(callback, handler, owner);
        } else {
            runtime.WriteCallbackWord(callback + 8u, handler);
            runtime.WriteCallbackWord(callback + 0xcu, 0u);
            runtime.WriteCallbackWord(callback + 0x10u, owner);
            runtime.WriteCallbackWord(callback, MainWindowElements::kCallbackVtable);
        }
    }
    ReleaseReference(runtime.SubscribeClicked(window, callback), runtime);
}

}  // namespace

std::uint32_t MainWindowElements::Initialize(
    MainWindowState& state, std::uint32_t object_address,
    IMainWindowElementsRuntime& runtime) {
    runtime.InitializeNameBuffer();
    try {
        for (std::size_t index = 0; index < kButtons.size(); ++index) {
            const auto& button = kButtons[index];
            const auto window = FindChild(state, button.name, runtime);
            state.Write(button.offset, window);
            Subscribe(window, object_address, button.handler, index < 5u,
                      runtime);
        }
        state.Write(0x64u, FindChild(state, "youxia", runtime));
    } catch (...) {
        (void)runtime.CleanupNameBuffer();
        throw;
    }
    return runtime.CleanupNameBuffer();
}

}  // namespace pal4
