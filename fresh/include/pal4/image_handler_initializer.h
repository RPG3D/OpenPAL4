#pragma once

#include "pal4/game_config_initializer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IImageHandlerRuntime : public IGameConfigLogger {
public:
    ~IImageHandlerRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t AddPath(
        std::string_view extension, std::uint32_t reader,
        std::uint32_t writer) = 0;
    virtual void SetTextureReadCallback() = 0;
};

class ImageHandlerInitializer final {
public:
    [[nodiscard]] std::uint32_t Initialize(
        IImageHandlerRuntime& runtime) const;
};

}  // namespace pal4
