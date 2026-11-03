#pragma once

#include "pal4/game_data_record_copy_runtime.h"

#include <cstdint>
#include <functional>

namespace pal4 {

class MagicDataRecordCopy final {
public:
    static constexpr std::uint32_t kRecordBytes = 152;

    // IDA 0x4f89f0: assign one source throughout a Magic record range.
    [[nodiscard]] static std::uint32_t AssignRepeated(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source, IGameDataRecordCopyRuntime& runtime);
    // IDA 0x4f8c50: assign a Magic record range backward.
    [[nodiscard]] static std::uint32_t AssignBackward(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IGameDataRecordCopyRuntime& runtime);
    // IDA 0x4f8ed0: copy-construct a Magic record in fresh storage.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t destination, std::uint32_t source,
        IGameDataRecordCopyRuntime& runtime);
    // IDA 0x4f96d0: assign one existing Magic record.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IGameDataRecordCopyRuntime& runtime);
    // IDA 0x4f9ea0: retain the first record of each equal run.
    [[nodiscard]] static std::uint32_t CopyUnique(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination,
        const std::function<bool(std::uint32_t, std::uint32_t)>& equal,
        IGameDataRecordCopyRuntime& runtime);
};

}  // namespace pal4
