#include "pal4/dialog_object.h"

#include <bit>
#include <cmath>

namespace pal4 {

bool DialogObject::StartPlayback(
    DialogObjectState& state, IDialogObjectStartRuntime& runtime) {
    const auto probability = std::bit_cast<float>(state.words[21u]);
    const auto sample = probability == 1.0f
        ? 1.0
        : static_cast<double>(runtime.Random15()) *
              std::bit_cast<float>(0x38000100u);
    const auto selected = !std::isnan(probability) && probability >= 0.0f &&
                          sample <= probability;
    auto& word = state.words[22u];
    word = (word & ~0xffu) | (selected ? 1u : 0u);
    return runtime.ResumePlayback(state) || runtime.PreparePlayback(state);
}

}  // namespace pal4
