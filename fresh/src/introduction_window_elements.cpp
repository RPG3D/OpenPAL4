#include "pal4/introduction_window_elements.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace pal4 {

namespace {

std::uint32_t FindChild(
    const IntroductionWindowState& state, std::string_view suffix,
    IIntroductionWindowElementsRuntime& runtime, bool final_append = false) {
    std::string name(runtime.LayoutPrefix(state));
    if (name.size() > std::numeric_limits<std::uint32_t>::max() ||
        suffix.size() >=
            std::numeric_limits<std::uint32_t>::max() - name.size()) {
        throw std::length_error("IntroductionWindow child name length");
    }
    if (!final_append ||
        runtime.AllocateFinalName(name.size() + suffix.size(), 0u)) {
        name.append(suffix);
    }
    return runtime.FindWindow(name);
}

void ReleaseReference(std::uint32_t reference,
                      IIntroductionWindowElementsRuntime& runtime) {
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteCallback(reference, 1u);
}

void Subscribe(std::uint32_t window, std::uint32_t object_address,
               std::uint32_t handler, bool manual,
               IIntroductionWindowElementsRuntime& runtime) {
    const auto storage = runtime.AllocateCallback(0x18u);
    std::uint32_t callback = 0u;
    if (storage != 0u) {
        if (manual) {
            runtime.InitializeManualCallback(
                storage, IntroductionWindowElements::kManualCallbackVtable,
                handler, 0u, object_address);
            callback = storage;
        } else {
            try {
                callback = runtime.ConstructCallback(
                    storage, handler, 0u, object_address);
            } catch (...) {
                runtime.FreeCallbackStorage(storage);
                throw;
            }
        }
    }
    ReleaseReference(runtime.SubscribeClicked(window, callback), runtime);
}

std::uint32_t ReleaseTemporary(
    IIntroductionWindowElementsRuntime& runtime) {
    const auto buffer = runtime.TemporaryBufferAddress();
    if (buffer == 0u) return 0u;
    const auto prefix = buffer - 1u;
    const auto count = runtime.ReadPrefixByte(prefix);
    if (count == 0u || count == 0xffu) {
        return runtime.FreeTemporary(prefix);
    }
    runtime.WritePrefixByte(prefix,
                            static_cast<std::uint8_t>(count - 1u));
    return prefix;
}

}  // namespace

std::uint32_t IntroductionWindowElements::Initialize(
    IntroductionWindowState& state, std::uint32_t object_address,
    std::uint8_t temporary_byte,
    IIntroductionWindowElementsRuntime& runtime) {
    runtime.InitializeTemporary(temporary_byte, 0u);
    bool temporary_released = false;
    const auto release_temporary = [&] {
        temporary_released = true;
        return ReleaseTemporary(runtime);
    };
    try {
        const auto close = FindChild(state, "BtnClose", runtime);
        state.Write(0x84u, close);
        Subscribe(close, object_address, kCloseHandler, false, runtime);

        const auto history = FindChild(state, "BtnHistory", runtime);
        state.Write(0x88u, history);
        runtime.SetWindowUserId(history, 0u);
        Subscribe(history, object_address, kMainHandler, false, runtime);

        const auto story = FindChild(state, "BtnStory", runtime);
        state.Write(0x8cu, story);
        Subscribe(story, object_address, kMainHandler, false, runtime);
        runtime.SetWindowUserId(story, 1u);
        Subscribe(story, object_address, kMainHandler, false, runtime);

        const auto worldview = FindChild(state, "BtnWorldview", runtime);
        state.Write(0x90u, worldview);
        Subscribe(worldview, object_address, kMainHandler, false, runtime);
        runtime.SetWindowUserId(worldview, 2u);
        Subscribe(worldview, object_address, kMainHandler, false, runtime);

        const auto sword = FindChild(state, "BtnSword", runtime);
        state.Write(0x94u, sword);
        runtime.SetWindowUserId(sword, 3u);
        Subscribe(sword, object_address, kMainHandler, true, runtime);

        for (std::uint32_t index = 1u; index <= 5u; ++index) {
            const auto name = std::string("BtnSub") + std::to_string(index);
            const auto window = FindChild(state, name, runtime);
            state.Write(0x94u + index * 4u, window);
            Subscribe(window, object_address, kSubHandler, true, runtime);
            runtime.SetWindowUserId(window, 0u);
        }

        state.Write(0x80u, FindChild(state, "EditInfo", runtime, true));
        state.Write(0xacu, FindChild(state, "PanelInfo", runtime, true));
        for (const auto offset : {0x88u, 0x8cu, 0x90u, 0x94u}) {
            runtime.SetupMouseHandlers(object_address + 0x28u,
                                       state.Read(offset));
        }
        state.Write(0xb4u, object_address);
        return release_temporary();
    } catch (...) {
        if (!temporary_released) (void)release_temporary();
        throw;
    }
}

}  // namespace pal4
