#include "pal4/six_word_block.h"

namespace pal4 {

std::uint32_t SixWordBlock::Initialize(
    SixWordBlockState& state, std::uint32_t object_address) {
    state.words.fill(0u);
    return object_address;
}

}  // namespace pal4
