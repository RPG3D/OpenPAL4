#pragma once
#include <cstdint>
#include <string_view>
#ifdef FindWindow
#undef FindWindow
#endif
namespace pal4 {
enum class TradeUiEvent { HoverRowChanged, MouseClick, ButtonClicked };

class ITradeWindowConstructionRuntime {
public:
    virtual ~ITradeWindowConstructionRuntime() = default;
    virtual void LoadLayout(std::uint32_t frame, std::uint32_t name) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void AssignText(std::uint32_t buffer, std::string_view text) = 0;
    virtual void ConstructNameNode(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree, std::uint32_t key,
                                 std::uint32_t callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(std::string_view name) = 0;
    virtual void SetSortDirection(std::uint32_t list,
                                  std::uint32_t direction) = 0;
    virtual void SetColumnDraggingEnabled(std::uint32_t list,
                                          bool enabled) = 0;
    virtual void SetColumnSizingEnabled(std::uint32_t list,
                                        bool enabled) = 0;
    virtual void SetUserSortEnabled(std::uint32_t list,
                                    bool enabled) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(std::uint32_t list) = 0;
    virtual void RemoveFirstColumn(std::uint32_t list) = 0;
    virtual void SetHorizontalFormatting(std::uint32_t window,
                                         std::uint32_t mode) = 0;
    virtual void SetFont(std::uint32_t window, std::string_view font) = 0;
    virtual void SetFormatting(std::uint32_t window,
                               std::uint32_t horizontal,
                               std::uint32_t vertical) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, TradeUiEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveImage(
        std::string_view imageset, std::string_view image) = 0;
    virtual void DestroyText(std::uint32_t buffer) = 0;
    virtual void DestroyNameNode(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(std::uint32_t frame) = 0;
};
}
