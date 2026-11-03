#pragma once

#include "pal4/gob_snapshot_runtime.h"

namespace pal4 {

class GobNormalStateSnapshot final {
public:
    static constexpr std::uint32_t kSize = 0x18u;
    static constexpr std::uint32_t kVtable = 0x849188u;
    static constexpr std::uint32_t kBaseVtable = 0x841e40u;

    [[nodiscard]] static std::uint32_t Create(IGobSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t snapshot, IGobSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t snapshot, IGobSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t WriteConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobSnapshotRuntime& runtime);
};

}  // namespace pal4
