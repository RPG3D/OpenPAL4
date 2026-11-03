#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameInfoWindowRuntime {
public:
    virtual ~IGameInfoWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object, std::uint32_t name) = 0;
    virtual void InitializeText(std::uint32_t address,
                                std::uint8_t initial_byte) = 0;
    virtual void AssignText(std::uint32_t address,
                            std::string_view text) = 0;
    virtual void DestroyText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view WindowName(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(
        std::uint32_t window, std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNameBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name_pointer) = 0;
    [[nodiscard]] virtual bool HandleShow(std::uint32_t name_pointer,
                                          std::uint32_t event) = 0;
    virtual void ProcessUiEvent(std::uint32_t tree, std::uint32_t key,
                                const std::array<std::uint32_t, 2>& data) = 0;
    [[nodiscard]] virtual std::int32_t InventoryMoney() = 0;
    [[nodiscard]] virtual std::string TriggerText(std::uint32_t id) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void SetText(std::uint32_t window, std::u32string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseFormatted(
        const std::string& text) = 0;
    virtual void SetParent(std::uint32_t object, bool visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ElapsedSeconds() = 0;
    [[nodiscard]] virtual std::uint32_t ClockOffsetSeconds() = 0;
    [[nodiscard]] virtual std::string FormatSeconds(
        std::uint32_t seconds) = 0;
    [[nodiscard]] virtual std::uint8_t ReleaseTimeString(
        const std::string& text) = 0;
};

class GameInfoWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x843f48u;
    static constexpr std::uint32_t kHelpCallbackVtable = 0x843f60u;
    static constexpr std::uint32_t kNamePointer = 0x8b139cu;
    static constexpr std::uint32_t kHelpEventKey = 0x8b1c30u;

    // IDA 0x4a66c0-0x4a7210.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeControls(
        std::uint32_t object, IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnHelpButtonClick(
        IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static bool HandleShow(std::uint32_t event,
                                         IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetHelpTipsId(
        std::uint32_t event, IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static bool HandleUpdate(IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateUi(
        std::uint32_t object, IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVisible(
        std::uint32_t object, bool visible, std::uint32_t parent,
        IGameInfoWindowRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdateClock(
        std::uint32_t object, IGameInfoWindowRuntime& runtime);
};

}  // namespace pal4
