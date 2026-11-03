#pragma once

#include <string>
#include <string_view>

namespace pal4 {

class PreviewTextCodec final {
public:
    [[nodiscard]] static std::u32string DecodeUtf8(std::string_view text);
};

}  // namespace pal4
