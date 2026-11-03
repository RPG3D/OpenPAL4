#include "pal4/ol_info_elements.h"

#include <string>

namespace pal4 {
namespace {

std::uint32_t FindChild(const OlInfoState& state,
                        std::string_view suffix,
                        IOlInfoElementsRuntime& runtime) {
    std::string name(runtime.LayoutPrefix(state));
    name.append(suffix);
    return runtime.FindWindow(name);
}

void ReleaseReference(std::uint32_t reference,
                      IOlInfoElementsRuntime& runtime) {
    if (reference == 0u) return;
    const auto count = runtime.ReferenceCount(reference) - 1u;
    runtime.SetReferenceCount(reference, count);
    if (count == 0u) runtime.DeleteReference(reference, 1u);
}

}  // namespace

std::uint32_t OlInfoElements::Initialize(
    OlInfoState& state, std::uint32_t object_address,
    IOlInfoElementsRuntime& runtime) {
    runtime.InitializeNameBuffer();
    try {
        const auto close = FindChild(state, "BtnClose", runtime);
        state.Write(0x50u, close);
        const auto callback = runtime.AllocateCallback(0x18u);
        if (callback != 0u) {
            runtime.WriteCallbackWord(callback + 8u, kCloseCallback);
            runtime.WriteCallbackWord(callback + 0xcu, 0u);
            runtime.WriteCallbackWord(callback + 0x10u, object_address);
            runtime.WriteCallbackWord(callback, kCallbackVtable);
        }
        ReleaseReference(runtime.SubscribeClicked(close, callback), runtime);

        state.Write(0x4cu, FindChild(state, "StaticDesc", runtime));
        for (std::uint32_t index = 0u; index < 8u; ++index) {
            const auto name = std::string("StaticInfo") +
                              std::to_string(index + 1u);
            state.Write(0x2cu + index * 4u,
                        FindChild(state, name, runtime));
        }
    } catch (...) {
        (void)runtime.CleanupNameBuffer();
        throw;
    }
    return runtime.CleanupNameBuffer();
}

}  // namespace pal4
