#include "pal4/object_movement_processor.h"
#include <bit>

namespace pal4 {
float ObjectCollisionCandidate::Time() const {
    return std::bit_cast<float>(words[1]);
}

ObjectCollisionCandidate& ObjectCollisionCandidate::InitializeBody() {
    for (std::size_t word = 2u; word != 8u; ++word)
        words[word] = 0u;
    return *this;
}

ObjectCollisionCandidate& ObjectCollisionCandidate::CopyFrom(
    const ObjectCollisionCandidate& source) {
    for (std::size_t word = 0u; word != words.size(); ++word)
        words[word] = source.words[word];
    return *this;
}

}  // namespace pal4
