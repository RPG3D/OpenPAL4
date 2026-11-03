#pragma once

#include "pal4/shop_ui_actions.h"

#include <cstdint>

namespace pal4 {

class IShopUiSmithRuntime : public IShopUiActionsRuntime {
public:
    ~IShopUiSmithRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t CreateDocument() = 0;
    [[nodiscard]] virtual std::uint32_t CreateTemporaryText() = 0;
    virtual void SetTextFlag(std::uint32_t text, std::uint8_t flag) = 0;
    virtual void AssignText(std::uint32_t text, std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t tree, std::uint32_t text) = 0;
    virtual void SetAttributeInt(std::uint32_t value,
                                 std::uint32_t number) = 0;
    [[nodiscard]] virtual std::uint32_t CreateAttribute(
        std::uint32_t name, std::uint32_t number) = 0;
    virtual void AddAttribute(std::uint32_t document,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeStateAndUi(std::uint32_t game,
                                      std::uint32_t state,
                                      std::uint32_t document) = 0;
    virtual void DestroyDocument(std::uint32_t document) = 0;
};

class ShopUiSmith final {
public:
    static constexpr std::uint32_t kAttributeNameSource = 0x8b94b8u;
    static constexpr std::uint32_t kSmithState = 5u;

    // IDA 0x486730.
    [[nodiscard]] static std::uint8_t Open(
        std::uint32_t frame, std::uint32_t event,
        IShopUiSmithRuntime& runtime);
};

}  // namespace pal4
