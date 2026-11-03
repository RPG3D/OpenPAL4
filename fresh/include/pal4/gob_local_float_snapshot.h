#pragma once

#include "pal4/gob_snapshot_runtime.h"

namespace pal4 {

class IGobLocalFloatSnapshotRuntime : public IGobSnapshotRuntime {
public:
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void WriteXmlFloat(std::uint32_t document,
                               std::string_view key, float value) = 0;
    [[nodiscard]] virtual float ReadXmlFloat(
        std::uint32_t document, std::string_view key,
        float fallback) = 0;
};

class GobLocalFloatStateSnapshot final {
public:
    static constexpr std::uint32_t kSize = 0x34u;
    static constexpr std::uint32_t kVtable = 0x848dbcu;
    static constexpr std::uint32_t kBaseVtable = 0x841e40u;

    [[nodiscard]] static std::uint32_t Create(
        IGobLocalFloatSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t snapshot, IGobLocalFloatSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t TypeKey();
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t snapshot, IGobLocalFloatSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t WriteConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobLocalFloatSnapshotRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadConfig(
        std::uint32_t snapshot, std::uint32_t stream,
        IGobLocalFloatSnapshotRuntime& runtime);
};

}  // namespace pal4
