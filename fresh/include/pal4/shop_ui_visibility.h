#pragma once

#include <cstdint>

namespace pal4 {

class IShopUiVisibilityRuntime {
public:
    virtual ~IShopUiVisibilityRuntime() = default;
    virtual void Update(std::uint32_t frame) = 0;
    virtual void SetParent(std::uint32_t frame, std::uint8_t visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertActive(std::uint32_t list, std::uint32_t item) = 0;
    virtual void RemoveActive(std::uint32_t list, std::uint32_t item) = 0;
    virtual void UpdateSystemDialogVisibility(std::uint32_t manager,
                                              std::uint32_t state) = 0;
};

class ShopUiVisibility final {
public:
    // IDA 0x486a70, the ShopUiFrame visibility vtable slot.
    static void SetVisible(std::uint32_t frame, std::uint8_t visible,
                           std::uint32_t parent,
                           IShopUiVisibilityRuntime& runtime);
};

}  // namespace pal4
