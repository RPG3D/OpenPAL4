#include "pal4/property_window_details.h"

namespace pal4 {

std::uint32_t PropertyWindowDetails::HoveredRecord(
    const PropertyWindowState& state,
    IPropertyWindowDetailsRuntime& runtime) {
    const auto item = runtime.HoverItem(state.Read(0x58u));
    if (item == 0u) return 0u;
    return runtime.FindInventoryElement(runtime.ReadWord(item + 0x138u), 5u);
}

std::uint32_t PropertyWindowDetails::Update(
    PropertyWindowState& state, std::uint32_t record,
    IPropertyWindowDetailsRuntime& runtime) {
    if (record == 0u) {
        runtime.SetText(state.Read(0x60u), "");
        runtime.SetText(state.Read(0x64u), "");
    } else {
        const auto definition = runtime.ReadWord(record);
        const auto description_address = runtime.ReadWord(definition + 0x24u);
        const auto description = runtime.ReadText(description_address != 0u
            ? description_address : kEmptyTextAddress);
        runtime.SetText(state.Read(0x60u), runtime.ConvertToUtf8(description));
        const auto property_address = runtime.ReadWord(definition + 0x34u);
        const auto property = runtime.ReadText(property_address != 0u
            ? property_address : kEmptyTextAddress);
        runtime.SetText(state.Read(0x64u), runtime.ConvertToUtf8(property));
    }
    const auto result = runtime.UpdateModel(state, record);
    state.Write(0x74u, record != 0u ? runtime.ReadWord(record + 4u) : 0u);
    return result;
}

}  // namespace pal4
