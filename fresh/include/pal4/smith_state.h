#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pal4 {

class ISmithStateRuntime {
public:
    virtual ~ISmithStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void AddUIElement(std::uint32_t manager,
                              std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> ReadIntegerAttribute(
        std::uint32_t config, std::string_view key) = 0;
    virtual void GetActiveCamera(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t UICameraName() = 0;
    virtual void ActivateCamera(std::uint32_t manager,
                                std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t SmithSystem() = 0;
    virtual void SetSmithFlag(std::uint32_t system, bool enabled) = 0;
    virtual void SetSmithState(std::uint32_t system,
                               std::uint32_t raw_delta) = 0;
    virtual void ClearSmithFlags(std::uint32_t system) = 0;
    virtual void UpdateUI(std::uint32_t manager, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t RenderEffectTransform(
        std::uint32_t manager, double delta) = 0;
    virtual void CleanupEffects(std::uint32_t manager) = 0;
};

class SmithState final {
public:
    static constexpr std::uint32_t kVtable=0x849468u;
    // IDA 0x590110/0x590130/0x590170/0x590190/0x590250.
    [[nodiscard]] static std::uint32_t EnterWrapper(
        std::uint32_t object, std::uint32_t unused,
        std::uint32_t config, ISmithStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t config, ISmithStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        float delta, std::uint32_t unused1,
        std::uint32_t unused2, ISmithStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveWrapper(
        std::uint32_t object, std::uint32_t unused,
        ISmithStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(
        ISmithStateRuntime& runtime);
};

}  // namespace pal4
