#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct PackageCodecState final {
    std::uint32_t mode = 0;
    std::uint8_t compress = 0;
    std::uint8_t reserved4[3]{};
    std::uint32_t source = 0;
    std::uint32_t destination = 0;
    std::uint8_t reserved16[8]{};
    std::uint32_t source_bytes = 0;
    std::uint32_t result_bytes = 0;
    std::uint8_t succeeded = 0;
    std::uint8_t reserved33[3]{};
};
static_assert(sizeof(PackageCodecState) == 36u);
static_assert(offsetof(PackageCodecState, source_bytes) == 24u);
static_assert(offsetof(PackageCodecState, succeeded) == 32u);

class IPackageCodecRuntime {
public:
    virtual ~IPackageCodecRuntime() = default;
    [[nodiscard]] virtual std::int32_t Compress(
        std::uint32_t source, std::uint32_t source_bytes,
        std::uint32_t destination, std::uint32_t& result_bytes) = 0;
    [[nodiscard]] virtual std::int32_t Decompress(
        std::uint32_t source, std::uint32_t source_bytes,
        std::uint32_t destination, std::uint32_t& result_bytes) = 0;
    [[nodiscard]] virtual std::int32_t ReportFailure(
        bool compress, std::int32_t lzo_result) = 0;
};

class PackageCodec final {
public:
    // IDA 0x794CF0. The runtime owns the MiniLZO and message-box calls.
    [[nodiscard]] static std::int32_t Run(PackageCodecState& state,
                                          IPackageCodecRuntime& runtime);
};

}  // namespace pal4
