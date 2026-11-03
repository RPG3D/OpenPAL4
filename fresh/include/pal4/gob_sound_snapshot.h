#pragma once

#include "pal4/gob_snapshot_runtime.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// Captures the base GOB name only while the inherited store flag is enabled.
class IGobSoundSnapshotCaptureRuntime {
public:
    virtual ~IGobSoundSnapshotCaptureRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual IGobSnapshotRuntime& SnapshotRuntime() = 0;
};

// IDA 0x5a6990..0x5a6bc0: 0x14-byte, name-only Sound state.
class GobSoundStateSnapshot final {
public:
    static constexpr std::uint32_t kSize = 0x14u;
    static constexpr std::uint32_t kVtable = 0x849300u;
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
