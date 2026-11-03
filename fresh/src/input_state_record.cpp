#include "pal4/input_state_record.h"

#include <algorithm>
#include <utility>

namespace pal4 {
namespace {

void WriteDword(std::array<std::uint8_t, InputStateRecord::kBytes>& bytes,
                const std::uint32_t offset, const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes[offset + index] =
            static_cast<std::uint8_t>(value >> (index * 8u));
    }
}

}  // namespace

InputStateRecord& InputStateRecord::Initialize() {
    for (const std::uint32_t offset : {0x18u, 0x1cu, 0x28u, 0x2cu}) {
        WriteDword(bytes_, offset, 0x7fffffffu);
    }
    for (const std::uint32_t offset : {0u, 4u, 8u,
                                       0x186cu, 0x1870u, 0x1874u}) {
        WriteDword(bytes_, offset, 0u);
    }
    WriteDword(bytes_, 0x0cu, 0x3f800000u);
    for (const std::uint32_t offset : {0x10u, 0x14u, 0x20u, 0x24u}) {
        WriteDword(bytes_, offset, 0x80000000u);
    }
    for (std::uint32_t offset = 0x38u; offset < 0x68u; offset += 4u) {
        WriteDword(bytes_, offset, 0u);
    }
    for (const auto [begin, end] : {
             std::pair{0x68u, 0x176u}, std::pair{0x176u, 0x284u},
             std::pair{0x278u, 0x6b0u}, std::pair{0x6b0u, 0xf20u}}) {
        std::fill(bytes_.begin() + begin, bytes_.begin() + end,
                  std::uint8_t{0});
    }
    return *this;
}

}  // namespace pal4
