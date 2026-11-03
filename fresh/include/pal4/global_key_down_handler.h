#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

// Screenshot/timer/XML services are independent of ordinary keyboard routing.
class IGlobalKeyDownSpecialRuntime {
public:
    virtual ~IGlobalKeyDownSpecialRuntime() = default;
    virtual void CaptureScreenshot() = 0;
    [[nodiscard]] virtual bool IsTimerFrameVisible(std::uint32_t manager) = 0;
    virtual void DispatchTimerEvent(std::uint32_t manager,
                                    std::uint32_t event_key,
                                    std::uint32_t active,
                                    std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t frame_key) = 0;
    [[nodiscard]] virtual std::uint32_t CreateDocument() = 0;
    [[nodiscard]] virtual std::uint32_t FindAttribute(
        std::uint32_t document, std::string_view name) = 0;
    virtual void SetAttributeInt(std::uint32_t attribute,
                                 std::uint32_t value) = 0;
    virtual void AddAttributeInt(std::uint32_t document,
                                 std::string_view name,
                                 std::uint32_t value) = 0;
    virtual void ChangeGameState(std::uint32_t state,
                                 std::uint32_t document) = 0;
    virtual void DestroyDocument(std::uint32_t document) = 0;
};

class IGlobalKeyDownRuntime : public IGlobalKeyDownSpecialRuntime {
public:
    virtual ~IGlobalKeyDownRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentModeObject() = 0;
    [[nodiscard]] virtual std::uint32_t QueryMode(std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t UiManager() = 0;
    [[nodiscard]] virtual std::uint32_t BinkPlayer() = 0;
    [[nodiscard]] virtual bool BinkIsPlaying(std::uint32_t player_state) = 0;
    virtual void CloseVideo(std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint8_t InvokeLastHandler(
        std::uint32_t handler, std::uint32_t event) = 0;
};

class GlobalKeyDownHandler final {
public:
    static constexpr std::uint32_t kTimerEventKey = 0x8b1c30u;
    static constexpr std::uint32_t kFrameKey = 0x8b13acu;

    // IDA 0x4aedd0; handler +4/+8 are pointer-array begin/end.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t handler, std::uint32_t event,
        IGlobalKeyDownRuntime& runtime);
};

}  // namespace pal4
