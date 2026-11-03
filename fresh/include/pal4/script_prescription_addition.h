#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptPrescriptionAdditionRuntime {
public:
    virtual ~IScriptPrescriptionAdditionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateHintWindow(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual bool HasRecord(
        std::uint32_t table, std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindRecord(
        std::uint32_t table, std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t Inventory() = 0;
    virtual void AddPrescription(std::uint32_t collection,
                                  std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeText() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t FindGameString(
        std::uint32_t table, std::uint32_t id) = 0;
    virtual void FormatText(std::uint32_t destination,
                            std::uint32_t format,
                            std::uint32_t name) = 0;
    virtual void AppendText(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void ShowHint(std::uint32_t hint, std::uint32_t text,
                          float x, float y, float scale,
                          std::uint32_t mode, std::uint32_t visible) = 0;
    [[nodiscard]] virtual std::uint32_t AudioManager() = 0;
    virtual void PlayMusic(std::uint32_t audio, std::uint8_t flag,
                           std::string_view path, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupText(
        std::uint32_t text) = 0;
};

class ScriptPrescriptionAddition final {
public:
    // IDA 0x5d8420, giAddPrescription.
    [[nodiscard]] static std::uint8_t Add(
        std::int32_t id, std::uint8_t show_hint,
        IScriptPrescriptionAdditionRuntime& runtime);
};

}  // namespace pal4
