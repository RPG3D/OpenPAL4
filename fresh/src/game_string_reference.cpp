#include "pal4/game_string_reference.h"

namespace pal4 {

std::uint32_t GameStringReference::FindInGameDB(
    IGameStringReferenceRuntime& runtime) const {
    const auto database = runtime.GameDbInstance();
    return runtime.FindGameString(database + 52, key_);
}

}  // namespace pal4
