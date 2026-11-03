#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IHintWindowRuntime {
public:
    virtual ~IHintWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t window,
                            std::string_view layout_name) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t string_record,
                              std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateText(
        std::uint32_t string_record, std::uint32_t length,
        std::uint8_t mode) = 0;
    virtual void CopyText(std::uint32_t destination,
                          std::string_view text) = 0;
    virtual void SetTextSize(std::uint32_t string_record,
                             std::uint32_t length) = 0;
    [[nodiscard]] virtual std::string GetNamePrefix(
        std::uint32_t field) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    virtual void SetAutoBreak(std::uint32_t window,
                              bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::uint32_t event,
        std::uint32_t handler) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

struct HintPosition {
    float x{};
    float y{};
};

class IHintWindowCloseRuntime {
public:
    virtual ~IHintWindowCloseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void RemoveChild(std::uint32_t parent, std::uint32_t root) = 0;
    [[nodiscard]] virtual int SetVisibility(std::uint32_t window,
                                           bool visible, std::uint32_t parent) = 0;
};

class IHintWindowUiRuntime : public IHintWindowCloseRuntime {
public:
    virtual ~IHintWindowUiRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view source) = 0;
    virtual void SetText(std::uint32_t edit,
                         std::u32string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t DefaultSheet() = 0;
    virtual void SetPosition(std::uint32_t root,
                             std::uint32_t metrics,
                             HintPosition position) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t root,
                                 bool enabled) = 0;
    virtual void PrepareLayout(std::uint32_t window) = 0;
    virtual void PrepareState(std::uint32_t window) = 0;
    virtual void RemoveQueued(std::uint32_t clock_record) = 0;
    virtual void QueueClose(std::uint32_t clock_record,
                            std::uint32_t duration,
                            std::uint32_t window,
                            std::uint32_t callback) = 0;
    [[nodiscard]] virtual float GetX(std::uint32_t root) = 0;
    [[nodiscard]] virtual float GetY(std::uint32_t root) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t root) = 0;
    [[nodiscard]] virtual float GetHeight(std::uint32_t root) = 0;
    virtual void SetX(std::uint32_t root, float x) = 0;
    virtual void SetY(std::uint32_t root, float y) = 0;
};

// Shared pixel-size operation used by ordinary and combat hints (IDA54A550).
class IHintWindowResizeRuntime {
public:
    virtual ~IHintWindowResizeRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual float TextHeight(std::uint32_t edit) = 0;
    virtual float MaxLineWidth(std::uint32_t edit) = 0;
    virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void SetHeight(std::uint32_t window,std::uint32_t mode,float value) = 0;
    virtual void SetWidth(std::uint32_t window,std::uint32_t mode,float value) = 0;
};

class HintWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x8440acu;
    static constexpr std::uint32_t kZOrderHandlerVtable = 0x8440a4u;
    static constexpr std::uint32_t kOnZOrderChanged = 0x4abed0u;
    static constexpr std::uint32_t kZOrderChangedEvent = 0x841308u;
    static constexpr std::string_view kClockName = "HintWindowClock";

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, std::string_view layout_name,
        IHintWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t window, IHintWindowRuntime& runtime);
    static void ConvertAndShow(std::uint32_t window,
                               std::string_view source,
                               HintPosition position,
                               std::uint32_t duration,
                               std::uint32_t parent,
                               std::uint8_t flag,
                               IHintWindowUiRuntime& runtime);
    static void Show(std::uint32_t window,
                     std::u32string_view text,
                     HintPosition position,
                     std::uint32_t duration,
                     std::uint32_t parent,
                     std::uint8_t flag,
                     IHintWindowUiRuntime& runtime);
    [[nodiscard]] static int Close(std::uint32_t window,
                                    IHintWindowCloseRuntime& runtime);
    static void Center(std::uint32_t window,
                       IHintWindowUiRuntime& runtime);
    [[nodiscard]] static bool ConditionalClose(
        std::uint32_t window, IHintWindowUiRuntime& runtime);
};

}  // namespace pal4
