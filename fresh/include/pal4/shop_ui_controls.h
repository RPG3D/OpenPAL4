#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

namespace pal4 {

class IShopUiControlsRuntime {
public:
    virtual ~IShopUiControlsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeNameBuffer() = 0;
    virtual void FormatName(std::uint32_t buffer, std::uint32_t prefix,
                            std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer(
        std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructClickCallback(
        std::uint32_t storage, std::uint32_t function,
        std::uint32_t adjustment, std::uint32_t frame) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t BindClicked(
        std::uint32_t window, std::uint32_t callback,
        std::uint32_t clicked_event) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    [[nodiscard]] virtual std::pair<std::uint32_t, std::uint32_t> GetPositionBits(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(
        std::uint32_t collection, std::uint32_t trigger) = 0;
    [[nodiscard]] virtual std::uint32_t ConvertToUtf8(
        std::uint32_t text) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::uint32_t utf8) = 0;
};

class ShopUiControls final {
public:
    static constexpr std::uint32_t kClickCallbackVtable = 0x843d1cu;
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;

    // IDA 0x4859d0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t frame, IShopUiControlsRuntime& runtime);
};

}  // namespace pal4
