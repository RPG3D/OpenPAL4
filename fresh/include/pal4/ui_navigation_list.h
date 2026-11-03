#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IUiNavigationConstructionRuntime {
public:
    virtual ~IUiNavigationConstructionRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

// Selection/redraw needs only live list words, the window highlight byte and
// redraw. Keep this boundary usable without subscription/game-input services.
class IUiNavigationSelectionRuntime {
public:
    virtual ~IUiNavigationSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
};

class IUiNavigationActivationRuntime : public IUiNavigationSelectionRuntime {
public:
    ~IUiNavigationActivationRuntime() override=default;
    virtual void SetButtonSelected(std::uint32_t,bool)=0;
};

class IUiNavigationInputRuntime : public IUiNavigationActivationRuntime,
                                  public IUiNavigationConstructionRuntime {
public:
    ~IUiNavigationInputRuntime() override = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) override = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void SetButtonSelected(std::uint32_t window, bool selected) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeCycle(
        std::uint32_t list, std::uint32_t slot,
        std::uint32_t previous_index) = 0;
    virtual void MouseAtCenter(std::uint32_t window) = 0;
};

class IUiNavigationSetupRuntime {
public:
    virtual ~IUiNavigationSetupRuntime()=default;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual void AppendWindow(std::uint32_t list, std::uint32_t window) = 0;
    virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual std::uint32_t Subscribe(std::uint32_t window,std::string_view event_name,std::uint32_t handler) = 0;
    virtual std::uint32_t ReleaseSubscription(std::uint32_t reference) = 0;
};
class IUiNavigationHoverRuntime : public IUiNavigationInputRuntime {
public:
    ~IUiNavigationHoverRuntime() override=default;
    virtual std::uint32_t UiManager()=0;
    virtual void NotifyHoverHandlers(std::uint32_t handler_array)=0;
    virtual std::uint32_t InvokeList(std::uint32_t list,std::uint32_t slot)=0;
};
class IUiNavigationRuntime : public IUiNavigationHoverRuntime, public IUiNavigationSetupRuntime {
public:
    ~IUiNavigationRuntime() override = default;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) override=0;
    virtual void AppendWindow(std::uint32_t list, std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::string_view event_name,
        std::uint32_t handler) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseSubscription(
        std::uint32_t reference) = 0;
    [[nodiscard]] virtual std::uint32_t UiManager() = 0;
    virtual void NotifyHoverHandlers(std::uint32_t handler_array) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeList(
        std::uint32_t list, std::uint32_t slot) = 0;
};

class UiNavigationList final {
public:
    static constexpr std::uint32_t kVtable = 0x8440d8u;

    // IDA 0x4af250.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t list, IUiNavigationConstructionRuntime& runtime);
    // IDA 0x4af270.
    [[nodiscard]] static std::uint8_t SelectCurrent(
        std::uint32_t list, IUiNavigationActivationRuntime& runtime);
    // IDA 0x4af2c0.
    [[nodiscard]] static std::uint8_t RedrawAll(
        std::uint32_t list, IUiNavigationSelectionRuntime& runtime);
    // IDA 0x4af2f0.
    [[nodiscard]] static std::uint8_t HandleKey(
        std::uint32_t list, std::uint32_t event,
        IUiNavigationInputRuntime& runtime);
    // IDA 0x4af390.
    static void SetSelected(std::uint32_t list, std::uint32_t window,
                            IUiNavigationSelectionRuntime& runtime);
    // IDA 0x4af3d0/0x4af450.
    [[nodiscard]] static std::uint8_t CycleNext(
        std::uint32_t list, std::uint32_t previous_index,
        IUiNavigationInputRuntime& runtime);
    [[nodiscard]] static std::uint8_t CyclePrevious(
        std::uint32_t list, std::uint32_t previous_index,
        IUiNavigationInputRuntime& runtime);
    // IDA 0x4af4d0.
    [[nodiscard]] static std::uint32_t SetupMouseHandlers(
        std::uint32_t list, std::uint32_t window,
        IUiNavigationSetupRuntime& runtime);
    // IDA 0x4af5b0/0x4af640.
    [[nodiscard]] static std::uint8_t OnHoveredAndNotify(
        std::uint32_t list, std::uint32_t event,
        IUiNavigationHoverRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnHoveredAndTrack(
        std::uint32_t list, std::uint32_t event,
        IUiNavigationHoverRuntime& runtime);
    // IDA 0x4af6a0.
    [[nodiscard]] static std::uint8_t HandleVerticalKey(
        std::uint32_t list, std::uint32_t event,
        IUiNavigationInputRuntime& runtime);
    // IDA 0x4af760/0x4af770.
    [[nodiscard]] static std::uint32_t ForwardMouseMove(
        std::uint32_t list, IUiNavigationHoverRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardMouseButtonUp(
        std::uint32_t list, IUiNavigationHoverRuntime& runtime);
};

}  // namespace pal4
