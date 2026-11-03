#include "pal4/select_dialog.h"

#include "pal4/preview_text_codec.h"

#include <cstdint>
#include <string>

namespace pal4 {

std::uint32_t SelectDialog::SetQuestionAndOptions(
    const std::uint32_t object, const std::string& source,
    const std::uint32_t option,
    ISelectDialogTextRuntime& runtime) {
    const auto utf8 = runtime.ConvertToUtf8(source);
    const auto text = PreviewTextCodec::DecodeUtf8(utf8);
    runtime.InitializeList(object, text, option);
    return runtime.DestroyCeguiString();
}

}  // namespace pal4
