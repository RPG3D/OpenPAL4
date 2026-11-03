#pragma once

#include "pal4/native_game_save_archive_buffer.h"

#include <cstdint>

namespace pal4 {

class NativeInventorySession;

// Appends the four player slots and P_MGR document to an open save stream.
class NativePlayerManagerSaveRuntime final {
public:
    explicit NativePlayerManagerSaveRuntime(NativeGameSaveArchiveBuffer& archive)
        : archive_(archive) {}

    [[nodiscard]] bool Serialize(std::uint32_t manager,
                                 NativeInventorySession& inventory);
    [[nodiscard]] bool SerializeManagerDocument(std::uint32_t manager);

private:
    NativeGameSaveArchiveBuffer& archive_;
};

}  // namespace pal4
