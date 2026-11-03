#pragma once

#include "pal4/native_game_save_archive_buffer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class NativeArenaConfigSaveRuntime final {
public:
    explicit NativeArenaConfigSaveRuntime(NativeGameSaveArchiveBuffer& archive)
        : archive_(archive) {}

    [[nodiscard]] bool Serialize(std::string_view world,
                                 std::string_view area,
                                 std::string_view diff);
    [[nodiscard]] bool SerializeArena(std::uint32_t arena);

private:
    NativeGameSaveArchiveBuffer& archive_;
};

}  // namespace pal4
