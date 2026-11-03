#pragma once

#include "pal4/global_key_down_handler.h"

#include <cstdint>
#include <functional>

namespace pal4 {

class NativeEventHandlerArray;
class NativePalGameIVStateList;

// The movie owner and callbacks must outlive this borrowed binding. The opaque
// address identifies the native player; no original Bink object layout is read.
struct NativeGlobalKeyDownMovie {
    std::uint32_t player;
    std::function<bool()> playing;
    std::function<void()> close;
};

// Reuses 4AEDD0 with real game/state/array storage, never original EXE vtables.
// Optional specialist services fail explicitly when their branch is requested.
class NativeGlobalKeyDown final : private IGlobalKeyDownRuntime {
public:
    NativeGlobalKeyDown(std::uint32_t game, NativePalGameIVStateList& states,
                        std::uint32_t manager, NativeEventHandlerArray& active,
                        NativeGlobalKeyDownMovie movie,
                        IGlobalKeyDownSpecialRuntime* special = nullptr);
    [[nodiscard]] std::uint8_t Process(std::uint32_t event);

private:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t CurrentModeObject() override;
    std::uint32_t QueryMode(std::uint32_t mode) override;
    std::uint32_t UiManager() override;
    std::uint32_t BinkPlayer() override;
    bool BinkIsPlaying(std::uint32_t state) override;
    void CloseVideo(std::uint32_t player) override;
    std::uint8_t InvokeLastHandler(std::uint32_t handler,
                                  std::uint32_t event) override;
    IGlobalKeyDownSpecialRuntime& Special() const;
    void CaptureScreenshot() override;
    bool IsTimerFrameVisible(std::uint32_t manager) override;
    void DispatchTimerEvent(std::uint32_t manager, std::uint32_t key,
                            std::uint32_t active, std::uint32_t frame) override;
    std::uint32_t FindFrame(std::uint32_t manager, std::uint32_t key) override;
    std::uint32_t CreateDocument() override;
    std::uint32_t FindAttribute(std::uint32_t document, std::string_view name) override;
    void SetAttributeInt(std::uint32_t attribute, std::uint32_t value) override;
    void AddAttributeInt(std::uint32_t document, std::string_view name,
                         std::uint32_t value) override;
    void ChangeGameState(std::uint32_t state, std::uint32_t document) override;
    void DestroyDocument(std::uint32_t document) override;

    std::uint32_t game_;
    NativePalGameIVStateList& states_;
    std::uint32_t manager_;
    NativeEventHandlerArray& active_;
    NativeGlobalKeyDownMovie movie_;
    IGlobalKeyDownSpecialRuntime* special_;
};

}  // namespace pal4
