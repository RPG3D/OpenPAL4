#include "pal4/smith_window.h"

#include <string>

namespace pal4 {

void SmithWindow::UpdateEquipmentItemDescription(
    const std::uint32_t object, const std::uint32_t item,
    ISmithWindowItemTextRuntime& runtime) {
    if (item == 0u) {
        runtime.SetText(runtime.ReadWord(object + 0x4cu), {});
        return;
    }

    std::string description(runtime.DescribeItem(item));
    const auto extra = runtime.ReadText(runtime.ReadWord(item + 0xbcu));
    if (!extra.empty()) {
        description += "<br/><colour red=0 green=0 blue=0 alpha=255>(";
        description += extra;
        description += ")</colour>";
    }
    const auto utf8 = runtime.Utf8Buffer(description);
    runtime.SetText(runtime.ReadWord(object + 0x4cu), utf8);
}

}  // namespace pal4
