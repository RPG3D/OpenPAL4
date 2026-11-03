#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

enum class MagicPanelEvent { Clicked, MouseMove, MouseClick };

struct MagicWindowPanelState {
    std::array<std::uint32_t, 0x170u / 4u> windows{};

    std::uint32_t& At(std::uint32_t object_offset) {
        return windows.at(object_offset / 4u);
    }
    const std::uint32_t& At(std::uint32_t object_offset) const {
        return windows.at(object_offset / 4u);
    }
};

class IMagicWindowPanelRuntime {
public:
    virtual ~IMagicWindowPanelRuntime() = default;
    virtual std::uint32_t FindWindow(std::string_view full_name) = 0;
    virtual void SetUserIndex(std::uint32_t window, std::uint32_t index) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    virtual std::uint32_t AllocateCallback(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual std::uint32_t Subscribe(std::uint32_t window,
                                    MagicPanelEvent event,
                                    std::uint32_t callback) = 0;
    virtual void ReleaseReference(std::uint32_t reference) = 0;
    virtual void SetupMouseHandlers(std::uint32_t member,
                                    std::uint32_t window) = 0;
    virtual void SetButtonTextColorsBlack(std::uint32_t window) = 0;
    virtual void ClearButtonField(std::uint32_t window) = 0;
    virtual void SetHoverTextColorFromNormal(std::uint32_t window) = 0;
    virtual void SetButtonField(std::uint32_t window,
                                std::uint32_t value) = 0;
    virtual void SetStaticTextColorsBlack(std::uint32_t window) = 0;
};

class MagicWindowPanelRuntimeStub : public IMagicWindowPanelRuntime {
public:
    std::uint32_t FindWindow(std::string_view) override { return 0; }
    void SetUserIndex(std::uint32_t, std::uint32_t) override {}
    void SetAlwaysOnTop(std::uint32_t, bool) override {}
    std::uint32_t AllocateCallback(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t Subscribe(std::uint32_t, MagicPanelEvent,
                            std::uint32_t) override { return 0; }
    void ReleaseReference(std::uint32_t) override {}
    void SetupMouseHandlers(std::uint32_t, std::uint32_t) override {}
    void SetButtonTextColorsBlack(std::uint32_t) override {}
    void ClearButtonField(std::uint32_t) override {}
    void SetHoverTextColorFromNormal(std::uint32_t) override {}
    void SetButtonField(std::uint32_t, std::uint32_t) override {}
    void SetStaticTextColorsBlack(std::uint32_t) override {}
};

class MagicWindowPanel final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843994u;
    static constexpr std::uint32_t kStuntClickHandler = 0x459ae0u;
    static constexpr std::uint32_t kMagicHoverHandler = 0x459bb0u;
    static constexpr std::uint32_t kMagicPointClickHandler = 0x459780u;
    static constexpr std::uint32_t kStuntButtonClickHandler = 0x459c00u;
    static constexpr std::uint32_t kStuntButtonHoverHandler = 0x459bd0u;
    static constexpr std::uint32_t kPreviousCharacterHandler = 0x4596e0u;
    static constexpr std::uint32_t kNextCharacterHandler = 0x459730u;

    static void Initialize(std::uint32_t object, std::string_view prefix,
                           MagicWindowPanelState& state,
                           IMagicWindowPanelRuntime& runtime);
};

}  // namespace pal4
