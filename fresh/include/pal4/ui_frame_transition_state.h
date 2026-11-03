#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class IUiFrameTransitionRuntime {
public:
    virtual ~IUiFrameTransitionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> ReadIntegerAttribute(
        std::uint32_t config, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint8_t IsDialogVisible(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint8_t InitializeStateAndUI(
        std::uint32_t game, std::uint32_t state) = 0;
};

class UiFrameTransitionState final {
public:
    static constexpr std::uint32_t kVtable = 0x848634u;

    // IDA 0x58fe40/0x58fe50/0x58fec0/0x58fef0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IUiFrameTransitionRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadConfig(
        std::uint32_t object, std::uint32_t unused,
        std::uint32_t config, IUiFrameTransitionRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, IUiFrameTransitionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Reset(
        std::uint32_t object, std::uint32_t unused,
        IUiFrameTransitionRuntime& runtime);
};

}  // namespace pal4
