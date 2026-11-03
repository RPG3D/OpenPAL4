#include "pal4/shop_ui_smith.h"

namespace pal4 {

std::uint8_t ShopUiSmith::Open(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiSmithRuntime& runtime) {
    static_cast<void>(ShopUiActions::Close(frame, runtime));
    const auto document = runtime.CreateDocument();
    std::uint32_t key = 0u;
    bool key_active = false;
    std::uint32_t temporary_attribute = 0u;
    bool attribute_active = false;
    try {
        const auto name = runtime.ReadWord(kAttributeNameSource);
        key = runtime.CreateTemporaryText();
        key_active = true;
        runtime.SetTextFlag(key, 0u);
        runtime.AssignText(key, name);
        const auto found = runtime.FindAttribute(document + 0x14u, key);
        runtime.SetTextFlag(key, 1u);
        key_active = false;

        if (found == runtime.ReadWord(document + 0x18u) ||
            found + 0x1cu == 0u) {
            temporary_attribute = runtime.CreateAttribute(name, 0u);
            attribute_active = true;
            runtime.AddAttribute(document, temporary_attribute);
            runtime.DestroyAttribute(temporary_attribute);
            attribute_active = false;
        } else {
            runtime.SetAttributeInt(found + 0x1cu, 0u);
        }

        runtime.InitializeStateAndUi(
            runtime.GameInstance(), kSmithState, document);
    } catch (...) {
        if (attribute_active) {
            runtime.DestroyAttribute(temporary_attribute);
        }
        if (key_active) runtime.SetTextFlag(key, 1u);
        runtime.DestroyDocument(document);
        throw;
    }
    runtime.DestroyDocument(document);
    return 1u;
}

}  // namespace pal4
