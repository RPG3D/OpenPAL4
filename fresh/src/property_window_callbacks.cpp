#include "pal4/property_window_callbacks.h"

namespace pal4 {

bool PropertyWindowCallbacks::OnCategoryClick(
    PropertyWindowState& state, std::uint32_t event,
    IPropertyWindowCallbacksRuntime& runtime) {
    const auto button = runtime.ReadWord(event + 8u);
    state.Write(108u, runtime.ReadWord(button + 280u));
    return UpdatePanel(state, runtime);
}

bool PropertyWindowCallbacks::UpdatePanel(
    PropertyWindowState& state,
    IPropertyWindowCallbacksRuntime& runtime) {
    runtime.InsertActiveMember(state, 176u);
    const auto category = state.Read(108u);
    runtime.SetImage(state.Read(104u),
                     state.Read(152u + category * 4u));
    runtime.PopulateList(state, category, false);
    return true;
}

bool PropertyWindowCallbacks::OnItemClick(
    PropertyWindowState& state, std::uint32_t event,
    IPropertyWindowCallbacksRuntime& runtime) {
    if (runtime.ReadWord(event + 28u) == 0u)
        runtime.HandleSelection(state, event + 12u);
    return true;
}

}  // namespace pal4
