#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ITimedEventQueueRuntime;

class IPoetryResumeDialogRuntime {
public:
    virtual ~IPoetryResumeDialogRuntime() = default;
    virtual void LoadLayout(std::uint32_t dialog, std::uint32_t layout,
                            std::uint32_t first, std::uint32_t second) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t buffer,
                                            std::uint32_t length,
                                            std::uint32_t flag) = 0;
    virtual void CopyText(std::uint32_t buffer, std::string_view text) = 0;
    virtual void SetTextSize(std::uint32_t buffer,
                             std::uint32_t length) = 0;
    virtual void InitializeTemporaryString() = 0;
    virtual void CleanupTemporaryString() = 0;
    [[nodiscard]] virtual std::string_view ReadPrefix(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t dialog, std::string_view name) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void SetAlpha(std::uint32_t window, float alpha) = 0;
    [[nodiscard]] virtual float GetAlpha(std::uint32_t window) = 0;
    virtual void ScheduleReset(std::uint32_t dialog,
                               std::uint32_t name_address,
                               float delay) = 0;
    [[nodiscard]] virtual std::uint32_t CallVirtualSlot4(
        std::uint32_t dialog, std::uint32_t first,
        std::uint32_t second) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t dialog) = 0;
};

class PoetryResumeDialog final {
public:
    static constexpr std::uint32_t kVtable = 0x844228u;
    static constexpr std::uint32_t kWindowCount = 28u;

    // IDA 0x4b34f0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t dialog, std::uint32_t layout,
        IPoetryResumeDialogRuntime& runtime);
    // IDA 0x4b3660.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t dialog, IPoetryResumeDialogRuntime& runtime);
    // IDA 0x4b36c0.
    [[nodiscard]] static std::uint32_t Hide(
        std::uint32_t dialog, IPoetryResumeDialogRuntime& runtime);
    // IDA 0x4b3720. The incidental EAX value is not part of the UI behavior.
    static void Update(std::uint32_t dialog, double elapsed,
                       IPoetryResumeDialogRuntime& runtime);
    // IDA 0x4b3760.
    static void ResetState(std::uint32_t dialog,
                           IPoetryResumeDialogRuntime& runtime);
    // IDA 0x4b3770.
    static void UpdateAlpha(std::uint32_t dialog, float elapsed,
                            IPoetryResumeDialogRuntime& runtime);
    // IDA 0x4b3900.
    [[nodiscard]] static std::uint32_t FadeOut(
        std::uint32_t dialog, float elapsed,
        IPoetryResumeDialogRuntime& runtime);
    // IDA 0x4b3980.
    static void RegisterCallback(
        std::uint32_t queue, std::uint32_t name,
        std::uint32_t delay_bits, std::uint32_t callback_owner,
        std::uint32_t callback_function,
        ITimedEventQueueRuntime& runtime);
};

}  // namespace pal4
