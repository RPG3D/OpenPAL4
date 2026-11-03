#pragma once

#include "pal4/game_arena_scene_record.h"
#include "pal4/native_game_database.h"
#include "pal4/utilities_scene_table.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace pal4 {

class NativeSceneRecordCatalog final {
public:
    explicit NativeSceneRecordCatalog(const NativeGameDatabase& database);

    [[nodiscard]] std::span<const GameArenaSceneRecord> Records() const noexcept {
        return records_;
    }
    [[nodiscard]] const UtilitiesSceneRecord& Image(std::size_t index) const {
        return images_.at(index).words;
    }

private:
    struct OwnedImage {
        UtilitiesSceneRecord words{};
        std::array<std::unique_ptr<std::uint8_t[]>, 3u> text;
    };

    std::vector<OwnedImage> images_;
    std::vector<GameArenaSceneRecord> records_;
};

}  // namespace pal4
