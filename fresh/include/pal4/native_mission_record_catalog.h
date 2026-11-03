#pragma once

#include "pal4/mission_window_list.h"
#include <vector>

namespace pal4 {
class NativeGameDatabase;
class NativePalGameIVTextureStorage;

struct NativeMissionRecord {
    std::uint32_t id{};
    std::int32_t required{}, category{}, availability{};
    float progress{};
    std::string title, picture, description;
};

// Owns a typed snapshot of the loaded Mission table. No legacy string ownership.
// Selection reuses4DB280/4DB500; Assigned reads the live game's Story objects.
class NativeMissionRecordCatalog final {
public:
    explicit NativeMissionRecordCatalog(const NativeGameDatabase&);
    [[nodiscard]] const std::vector<NativeMissionRecord>& Records() const noexcept { return records_; }
    [[nodiscard]] std::vector<const NativeMissionRecord*> Sorted() const;
    [[nodiscard]] MissionRecordSlots Assigned(NativePalGameIVTextureStorage&) const;
private:
    std::vector<NativeMissionRecord> records_;
};
}
