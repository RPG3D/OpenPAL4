#pragma once

#include <cstdint>

namespace pal4 {

class IOpeningVideoRuntime {
public:
    virtual ~IOpeningVideoRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void SetUIFlag(std::uint32_t manager, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeConsole() = 0;
    [[nodiscard]] virtual std::uint32_t BinkPlayer() = 0;
    virtual void RenderVideo(std::uint32_t player, std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint8_t IsPlaying(std::uint32_t player_state) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ProcessScriptWaitHandlers(std::uint32_t queue, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t ScreenEffect() = 0;
    virtual void AdvanceScreenEffect(std::uint32_t effect, float delta) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructXmlDocument() = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeStateAndUI(std::uint32_t game,
                                      std::uint32_t state,
                                      std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyXmlDocument(
        std::uint32_t document) = 0;
};

class OpeningVideoController final {
public:
    static constexpr std::uint32_t kVtable = 0x8485b0u;
    static constexpr std::uint32_t kBaseVtable = 0x8485c8u;

    // IDA 0x58ac20 / 0x58ac30 / 0x58ac50 / 0x58ad90.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IOpeningVideoRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint32_t flags,
        IOpeningVideoRuntime& runtime);
    static void InitializeBase(std::uint32_t object,
                               IOpeningVideoRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t object, std::uint32_t flags,
        IOpeningVideoRuntime& runtime);

    // IDA 0x58ac60 / 0x58ac90 / 0x58acb0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t state,
        std::uint32_t unused, IOpeningVideoRuntime& runtime);
    static void Render(std::uint32_t record, IOpeningVideoRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, float delta, IOpeningVideoRuntime& runtime);
};

}  // namespace pal4
