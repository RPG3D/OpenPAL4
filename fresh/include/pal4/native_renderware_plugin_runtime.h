#pragma once

#include "pal4/game_config_initializer.h"
#include "pal4/renderware_plugin_initializer.h"

namespace pal4 {

class NativeRenderWarePluginRuntime final : public IRenderWarePluginRuntime {
public:
    explicit NativeRenderWarePluginRuntime(IGameConfigLogger& logger)
        : logger_(logger) {}

    [[nodiscard]] std::uint32_t Attach(RenderWarePlugin plugin) override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    void AppendLog(std::uint32_t logger, std::string_view format,
                   std::string_view source_file,
                   std::uint32_t source_line) override;

private:
    IGameConfigLogger& logger_;
};

}  // namespace pal4
