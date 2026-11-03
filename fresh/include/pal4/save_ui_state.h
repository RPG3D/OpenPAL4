#pragma once
#include "pal4/world_ui_frame.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISaveUiConstructionRuntime {
public:
    virtual ~ISaveUiConstructionRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ISaveUiEntryRuntime : public ISaveUiConstructionRuntime {
public:
    virtual ~ISaveUiEntryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    virtual void DispatchEvent(std::uint32_t manager,
                               std::uint32_t descriptor,
                               std::uint32_t first,
                               std::uint32_t second) = 0;
};

class ISaveUiUpdateRuntime {
public:
    virtual ~ISaveUiUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint8_t IsVisible(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint8_t InitializeStateAndUI(
        std::uint32_t game, std::uint32_t state) = 0;
};

class ISaveUiRuntime : public ISaveUiEntryRuntime,
                       public ISaveUiUpdateRuntime, public IWorldUiFrameRuntime {
public:
    virtual ~ISaveUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() override = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void ProcessWorldFrame(std::uint32_t arena,
                                   std::uint32_t frame_value) = 0;
    [[nodiscard]] virtual std::uint32_t RenderUI(
        std::uint32_t manager, std::uint32_t frame) = 0;
};

class ISaveUiExitRuntime {
public:
    virtual ~ISaveUiExitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetComponentProperty(
        std::uint32_t component, std::uint32_t property) = 0;
    virtual void SetComponentProperty(std::uint32_t component,
                                      std::uint32_t property,
                                      std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EffectManager() = 0;
    virtual void CreateEffect(std::uint32_t manager,
                              std::string_view name,
                              std::uint32_t position,
                              std::uint32_t selector,
                              std::uint32_t flags) = 0;
    virtual void ResetTemporaryEffectHandle() = 0;
};

class SaveUiState final {
public:
    static constexpr std::uint32_t kVtable = 0x84864cu;
    static constexpr std::uint32_t kShowSaveUiEvent = 0x8a7898u;

    // IDA 0x58ff30/0x58ff40/0x58ffa0/0x58ffd0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ISaveUiConstructionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Enter(
        std::uint32_t unused1, std::uint32_t unused2,
        ISaveUiEntryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Render(
        std::uint32_t frame, IWorldUiFrameRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        ISaveUiUpdateRuntime& runtime);
    // IDA 0x590010, virtual slot 0x84865c.
    [[nodiscard]] static std::uint32_t Finish(
        std::uint32_t unused, ISaveUiExitRuntime& runtime);
};

}  // namespace pal4
