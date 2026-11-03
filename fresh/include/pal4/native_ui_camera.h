#pragma once

struct RwFrame;

namespace pal4 {

class NativeUiCamera final {
public:
    [[nodiscard]] static bool ConfigureLogo(RwFrame* frame);
};

}  // namespace pal4
