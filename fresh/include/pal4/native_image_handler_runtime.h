#pragma once

#include "pal4/image_handler_initializer.h"
#include "pal4/rw_texture_read_hook_native.h"

#include <rwcore.h>

namespace pal4 {

class NativeImageHandlerRuntime final : public IImageHandlerRuntime {
public:
    NativeImageHandlerRuntime(IGameConfigLogger& logger,
                              RwImageCallBackRead tga_reader)
        : logger_(logger), tga_reader_(tga_reader), hook_runtime_(hook_state_) {}

    [[nodiscard]] std::uint32_t AddPath(
        std::string_view extension, std::uint32_t reader,
        std::uint32_t writer) override;
    void SetTextureReadCallback() override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    void AppendLog(std::uint32_t logger, std::string_view format,
                   std::string_view source_file,
                   std::uint32_t source_line) override;

private:
    IGameConfigLogger& logger_;
    RwImageCallBackRead tga_reader_;
    RwTextureReadHookState hook_state_{};
    NativeRwTextureReadHookRuntime hook_runtime_;
};

}  // namespace pal4
