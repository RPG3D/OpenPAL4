#include "pal4/ui_object_elements.h"

#include <limits>
#include <stdexcept>

namespace pal4 {

namespace {

std::uint32_t FindChild(
    UiObjectState& state, std::string_view suffix,
    IUiObjectElementsRuntime& runtime,
    bool website_append = false) {
    std::string name(runtime.LayoutPrefix(state));
    if (website_append) {
        if (suffix.size() >
            std::numeric_limits<std::size_t>::max() - name.size()) {
            throw std::length_error("UI object name length");
        }
        if (runtime.AllocateWebsiteName(name.size() + suffix.size(), 0u)) {
            name.append(suffix);
        }
    } else {
        name.append(suffix);
    }
    return runtime.FindWindow(name);
}

void ReleaseButtonReference(std::uint32_t reference,
                            IUiObjectElementsRuntime& runtime) {
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteCallback(reference, 1u);
}

std::uint32_t ReleaseTemporary(IUiObjectElementsRuntime& runtime) {
    const auto buffer = runtime.TemporaryBufferAddress();
    if (buffer == 0u) return 0u;
    const auto prefix = buffer - 1u;
    const auto count = runtime.ReadPrefixByte(prefix);
    if (count == 0u || count == 0xffu) {
        return runtime.FreeTemporary(prefix);
    }
    runtime.WritePrefixByte(prefix, static_cast<std::uint8_t>(count - 1u));
    return prefix;
}

}  // namespace

std::uint32_t UiObjectElements::Initialize(
    UiObjectState& state, std::uint32_t object_address,
    std::uint8_t temporary_byte, IUiObjectElementsRuntime& runtime) {
    runtime.InitializeTemporary(temporary_byte, 0u);
    bool temporary_released = false;
    const auto release_temporary = [&] {
        temporary_released = true;
        return ReleaseTemporary(runtime);
    };
    try {
        const auto close = FindChild(state, "BtnClose", runtime);
        state.Write(0x44u, close);
        const auto close_storage = runtime.AllocateCallback(0x18u);
        std::uint32_t close_callback = 0u;
        if (close_storage != 0u) {
            try {
                close_callback = runtime.ConstructCloseCallback(
                    close_storage, kCloseHandler, 0u, object_address);
            } catch (...) {
                runtime.FreeCallbackStorage(close_storage);
                throw;
            }
        }
        const auto close_reference = runtime.SubscribeClicked(close,
                                                               close_callback);
        if (close_reference != 0u) {
            runtime.ReleaseCloseReference(close_reference);
        }

        state.Write(0x40u, FindChild(state, "EditInfo", runtime));
        state.Write(0x50u, FindChild(state, "PanelInfo", runtime));

        const auto cast = FindChild(state, "BtnCast", runtime);
        state.Write(0x48u, cast);
        runtime.SetWindowUserId(cast, 17u);
        const auto cast_storage = runtime.AllocateCallback(0x18u);
        if (cast_storage != 0u) {
            runtime.InitializeButtonCallback(cast_storage, kCallbackVtable,
                                             kButtonHandler, 0u,
                                             object_address);
        }
        ReleaseButtonReference(runtime.SubscribeClicked(cast, cast_storage),
                               runtime);

        const auto website = FindChild(state, "BtnWebsite", runtime, true);
        state.Write(0x4cu, website);
        runtime.SetWindowUserId(website, 18u);
        const auto website_storage = runtime.AllocateCallback(0x18u);
        if (website_storage != 0u) {
            runtime.InitializeButtonCallback(website_storage, kCallbackVtable,
                                             kButtonHandler, 0u,
                                             object_address);
        }
        ReleaseButtonReference(
            runtime.SubscribeClicked(website, website_storage), runtime);

        runtime.SetupMouseHandlers(state, 0x28u, cast);
        runtime.SetupMouseHandlers(state, 0x28u, website);
        state.Write(0x58u, object_address);
        return release_temporary();
    } catch (...) {
        if (!temporary_released) (void)release_temporary();
        throw;
    }
}

}  // namespace pal4
