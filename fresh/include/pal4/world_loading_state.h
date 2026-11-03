#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IWorldLoadingStateRuntime {
public:
    virtual ~IWorldLoadingStateRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InitializeString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupString(
        std::uint32_t address) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
};

class IWorldLoadingConfigRuntime {
public:
    virtual ~IWorldLoadingConfigRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t DefaultCameraName() = 0;
    virtual void ActivateCamera(std::uint32_t manager,
                                std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t LoadingWindow(
        std::uint32_t manager) = 0;
    virtual void SetLoadingWindowVisible(std::uint32_t window,
                                          bool visible) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> ReadType(
        std::uint32_t config, std::string_view key) = 0;
    [[nodiscard]] virtual std::optional<std::string> ReadString(
        std::uint32_t config, std::string_view key) = 0;
    virtual void AssignString(std::uint32_t destination,
                              std::string_view value) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> ReadTreeInteger(
        std::uint32_t config, std::string_view key) = 0;
};

class IWorldLoadingFlowRuntime {
public:
    virtual ~IWorldLoadingFlowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RenderUI(
        std::uint32_t manager, std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t LoadingWindow(
        std::uint32_t manager) = 0;
    virtual void SetLoadingWindowVisible(std::uint32_t window,
                                         bool visible) = 0;
    virtual void LoadSave(std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStateAndUI(
        std::uint32_t game, std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void LoadSceneResources(std::uint32_t arena,
                                    std::uint32_t scene,
                                    std::uint32_t area) = 0;
    virtual void LoadSceneRuntime(std::uint32_t arena,
                                  std::uint32_t resources) = 0;
    virtual void InitializePlayerScene(std::uint32_t arena) = 0;
};

class WorldLoadingState final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8485c8u;
    static constexpr std::uint32_t kVtable = 0x84861cu;

    // IDA 0x58f3c0 / 0x58f460. Three 12-byte string buffers are at
    // offsets 12, 24 and 36 in the native 56-byte object.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IWorldLoadingStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IWorldLoadingStateRuntime& runtime);

    // IDA 0x58f4d0. The first native stack argument is unused.
    [[nodiscard]] static std::uint32_t LoadConfig(
        std::uint32_t object, std::uint32_t unused,
        std::uint32_t config, IWorldLoadingConfigRuntime& runtime);

    // IDA 0x58f800/0x58f820/0x58f8e0.
    [[nodiscard]] static std::uint32_t RenderUI(
        std::uint32_t frame, IWorldLoadingFlowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Advance(
        std::uint32_t object, IWorldLoadingFlowRuntime& runtime);
    [[nodiscard]] static std::uint32_t HideLoadingWindow(
        std::uint32_t unused, IWorldLoadingFlowRuntime& runtime);
    [[nodiscard]] static std::uint32_t HideLoadingWindow(
        std::uint32_t unused, IWorldLoadingConfigRuntime& runtime);
};

}  // namespace pal4
