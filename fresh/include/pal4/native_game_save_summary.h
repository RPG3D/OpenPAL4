#pragma once
#include "pal4/native_game_save_archive_reader.h"

namespace pal4 {
struct NativeGameSaveSummary {
    std::string marker;
    GameSaveHeader header;
};
// Quick-load prefix view over the same reader used for full restoration.
class NativeGameSaveSummaryReader final {
public:
    [[nodiscard]] std::optional<NativeGameSaveSummary> ReadSlot(
        const std::filesystem::path& game_root, std::int32_t slot);
    [[nodiscard]] const std::string& LastError() const noexcept { return reader_.LastError(); }
private:
    NativeGameSaveArchiveReader reader_;
};
} // namespace pal4
