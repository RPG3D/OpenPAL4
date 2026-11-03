#pragma once

#include "pal4/cegui_text_record_array.h"

#include <cstdint>

namespace pal4 {

class TradeWindowItemSnapshot final {
public:
    static constexpr std::uint32_t kBytes = 0x190u;
    static constexpr std::uint32_t kTextOffset = 0x98u;
    static constexpr std::uint32_t kTextCount = 13u;

    // IDA 0x491260.
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        ICeguiTextRecordRuntime& runtime);
};

}  // namespace pal4
