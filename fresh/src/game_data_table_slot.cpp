#include "pal4/game_data_table_slot.h"

namespace pal4 {

GameDataTableSlot& GameDataTableSlot::Initialize() noexcept {
    words.fill(0);
    return *this;
}

}  // namespace pal4
