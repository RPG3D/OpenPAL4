#include "pal4/dialog_layout_base.h"

#include "pal4/preview_text_codec.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::uint32_t DialogLayoutBase::GetWindow(
    const std::string& source,
    IDialogLayoutLookupRuntime& runtime) {
    return GetWindowByName(runtime.ConvertToUtf8(source), runtime);
}

std::uint32_t DialogLayoutBase::GetWindowByName(
    const std::string& utf8,
    IDialogLayoutLookupRuntime& runtime) {
    return runtime.GetWindow(PreviewTextCodec::DecodeUtf8(utf8));
}

std::uint32_t DialogLayoutBase::InvokeVirtual12(
    const std::uint32_t object,
    IDialogLayoutVirtualRuntime& runtime) {
    return runtime.Invoke(object, 12u);
}

std::uint32_t DialogLayoutBase::InvokeVirtual16(
    const std::uint32_t object,
    IDialogLayoutVirtualRuntime& runtime) {
    return runtime.Invoke(object, 16u);
}

std::uint32_t DialogLayoutBase::InvokeVirtual20(
    const std::uint32_t object,
    IDialogLayoutVirtualRuntime& runtime) {
    return runtime.Invoke(object, 20u);
}

}  // namespace pal4
