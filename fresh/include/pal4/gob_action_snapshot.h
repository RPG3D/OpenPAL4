#pragma once

#include "pal4/gob_snapshot_runtime.h"

namespace pal4 {

using IGobActionSnapshotRuntime = IGobSnapshotRuntime;

class GobActionStateSnapshot final {
public:
    static constexpr std::uint32_t kSize = 0x1cu;
    static constexpr std::uint32_t kVtable = 0x848b4cu;
    static constexpr std::uint32_t kBaseVtable = 0x841e40u;

    [[nodiscard]] static std::uint32_t Create(
        IGobActionSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t snapshot, IGobActionSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t TypeKey();
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t snapshot, IGobActionSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t WriteConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobActionSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobActionSnapshotRuntime& runtime);
};

}  // namespace pal4
