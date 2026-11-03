#pragma once

#undef ChangeDisplaySettings

#include "pal4/game_render_system.h"
#include "pal4/game_systems_shutdown.h"
#include "pal4/native_renderware_game_systems_runtime.h"

#undef ChangeDisplaySettings

namespace pal4 {

class NativeGameSystemsShutdownRuntime final
    : public IGameSystemsShutdownRuntime,
      private IRenderWareApi,
      private IGameDisplayModeRuntime {
public:
    NativeGameSystemsShutdownRuntime(
        GameRenderState& state,
        NativeRenderWareGameSystemsRuntime& renderware,
        IGameShutdownLogger& logger)
        : system_(state), renderware_(renderware), logger_(logger) {}

    void ShutdownRenderSystem() override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    [[nodiscard]] std::int32_t AppendLog(
        std::uint32_t logger, std::string_view format,
        std::string_view source_file, std::uint32_t source_line) override;
    [[nodiscard]] std::uint32_t DestroyedDictionaryCount() const {
        return destroyed_dictionary_count_;
    }

private:
    void DestroyTextureDictionary(std::uint32_t dictionary) override;
    void CloseCharset() override;
    void ForEachTextureDictionary(GameRenderSystem& system,
                                  std::uint32_t data) override;
    void CloseFileSystemManager() override;
    void EngineStop() override;
    void EngineClose() override;
    void EngineShutdown() override;
    [[nodiscard]] std::int32_t ChangeDisplaySettings(
        std::uint32_t mode, std::uint32_t flags) override;

    GameRenderSystem system_;
    NativeRenderWareGameSystemsRuntime& renderware_;
    IGameShutdownLogger& logger_;
    std::uint32_t destroyed_dictionary_count_ = 0u;
};

}  // namespace pal4
