#pragma once

#include "pal4/native_game_save_archive_buffer.h"

#include <cstdint>

namespace pal4 {

class NativePlayerBasicSaveRuntime final {
public:
    explicit NativePlayerBasicSaveRuntime(NativeGameSaveArchiveBuffer& archive)
        : archive_(archive) {}

    [[nodiscard]] bool Serialize(std::uint32_t player_slot);

private:
    NativeGameSaveArchiveBuffer& archive_;
};

}  // namespace pal4
