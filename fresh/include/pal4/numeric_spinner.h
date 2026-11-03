#pragma once

#include <cstdint>
#include <array>
#include <string_view>

namespace pal4 {

class INumericSpinnerRuntime {
public:
    virtual ~INumericSpinnerRuntime() = default;
    virtual void ConstructListboxItem(std::uint32_t item,
                                      std::string_view text) = 0;
    [[nodiscard]] virtual std::uint8_t InitialNameByte() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetErrorFlag(std::uint32_t name, std::uint8_t value) = 0;
    virtual void ReallocateName(std::uint32_t name,
                                std::uint32_t length) = 0;
    virtual void CopyName(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void CreateControls(std::uint32_t item) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void DestroyWindow(std::uint32_t window) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BaseItemVtable() = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    virtual void SetRect(std::uint32_t window,
                         const std::array<float, 4>& rect) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(float value) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t window) = 0;
    virtual void SetX(std::uint32_t window, float value) = 0;
    virtual void SetY(std::uint32_t window, float value) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    virtual void SetText(std::uint32_t window,
                         std::string_view text) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window,
                                           bool inherited) = 0;
    virtual void SetItemText(std::uint32_t item,
                             std::string_view text) = 0;
    virtual void InvokeChangeCallback(std::uint32_t callback) = 0;
    virtual void SelectGridCell(std::uint32_t list,
                                std::uint32_t row,
                                std::uint32_t column,
                                bool selected) = 0;
    virtual void DispatchUiEvent(std::uint32_t key,
                                 std::uint32_t row,
                                 std::uint32_t column) = 0;
};

struct SpinnerImage {
    std::uint32_t handle;
    float width;
    float height;
};

enum class SpinnerImageState { Normal, Pushed, Disabled, Hover };
enum class SpinnerMouseEvent { Clicked, ButtonDown, ButtonUp };
enum class SpinnerCallback {
    LeftClick, RightClick, LeftDown, RightDown, ButtonUp
};

class INumericSpinnerControlsRuntime {
public:
    virtual ~INumericSpinnerControlsRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadName(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateWindow(
        std::string_view type, std::string_view name) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void SetStandardImagery(std::uint32_t button,
                                    bool enabled) = 0;
    [[nodiscard]] virtual SpinnerImage FindImage(
        std::string_view imageset, std::string_view image) = 0;
    virtual void SetButtonImage(std::uint32_t button,
                                SpinnerImageState state,
                                std::uint32_t image) = 0;
    virtual void SetHeight(std::uint32_t window, float height) = 0;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
    virtual void SetBlackText(std::uint32_t window) = 0;
    virtual void SetFormatting(std::uint32_t window,
                               std::uint32_t horizontal,
                               std::uint32_t vertical) = 0;
    virtual void Subscribe(std::uint32_t window,
                           SpinnerMouseEvent event,
                           SpinnerCallback callback,
                           std::uint32_t owner) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
    virtual void UpdateValue(std::uint32_t item,
                             std::int32_t value) = 0;
};

class NumericSpinner final {
public:
    static constexpr std::uint32_t kVtable = 0x844128u;

    // IDA 0x4b0120.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t item, std::string_view name,
        std::int32_t initial_value, INumericSpinnerRuntime& runtime);
    // IDA 0x4b0330: an empty three-argument callback shared by vtables.
    static void NoopThreeArgs(std::uint32_t, std::uint32_t,
                              std::uint32_t) noexcept;
    // IDA 0x4b0340: copies two dwords, not the value at +440.
    [[nodiscard]] static std::uint32_t CopyPair(
        std::uint32_t item, std::uint32_t output,
        INumericSpinnerRuntime& runtime);
    // IDA 0x4b0380.
    static void Destruct(std::uint32_t item,
                         INumericSpinnerRuntime& runtime);
    // IDA 0x4b04c0; only the two accessed rectangle floats are used.
    static void Layout(std::uint32_t item, float compare,
                       const std::array<float, 4>& rect,
                       INumericSpinnerRuntime& runtime);
    // IDA 0x4b06b0.
    static void Reset(std::uint32_t item,
                      INumericSpinnerRuntime& runtime);
    // IDA 0x4b07d0.
    static void RemoveFromParent(std::uint32_t item,
                                 INumericSpinnerRuntime& runtime);
    // IDA 0x4b07f0; CEGUI object and callback ownership stays in runtime.
    static void CreateControls(std::uint32_t item,
                               INumericSpinnerControlsRuntime& runtime);
    // IDA 0x4b1550.
    static void UpdateValue(std::uint32_t item, std::int32_t value,
                            INumericSpinnerRuntime& runtime);
    // IDA 0x4b1710-0x4b1850.
    [[nodiscard]] static std::uint8_t OnLeftClick(
        std::uint32_t item, INumericSpinnerRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnRightClick(
        std::uint32_t item, INumericSpinnerRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnLeftDown(
        std::uint32_t item, std::uint32_t event,
        INumericSpinnerRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnRightDown(
        std::uint32_t item, std::uint32_t event,
        INumericSpinnerRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnButtonUp(
        std::uint32_t item, INumericSpinnerRuntime& runtime);
};

}  // namespace pal4
