#pragma once

#include <cstdint>

namespace pal4 {

class ITradeStateRuntime {
public:
    virtual ~ITradeStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameCamera(std::uint32_t frame) { return ReadWord(frame); }
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void ProcessWorldFrame(std::uint32_t arena,
                                   std::uint32_t frame_value) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    virtual void PreRenderUI(std::uint32_t manager,
                             std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t RenderUI(
        std::uint32_t manager, std::uint32_t frame) = 0;
    virtual void UpdateUI(std::uint32_t manager, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ProcessWaitHandlers(std::uint32_t handlers, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t MainCameraName() = 0;
    [[nodiscard]] virtual std::uint32_t UICameraName() = 0;
    [[nodiscard]] virtual std::uint32_t ActivateCamera(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t RenderEffectTransform(
        std::uint32_t manager, double delta) = 0;
    virtual void CloseShop(std::uint32_t system) = 0;
    virtual void CleanupEffects(std::uint32_t manager) = 0;
};

class TradeState final {
public:
    static constexpr std::uint32_t kVtable = 0x849450u;
    // IDA 0x590460..0x590590, slots 0x849454..0x849460.
    [[nodiscard]] static std::uint32_t EnterWrapper(
        std::uint32_t unused1, std::uint32_t unused2,
        ITradeStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Enter(ITradeStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Render(
        std::uint32_t frame, ITradeStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        float delta, std::uint32_t unused1,
        std::uint32_t unused2, ITradeStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveWrapper(
        std::uint32_t unused, ITradeStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Leave(ITradeStateRuntime& runtime);
};

}  // namespace pal4
