#pragma once

#include "pal4/archive_file_stream.h"

#include <cstdint>

namespace pal4 {

class IArchiveHandleStreamRuntime {
public:
    virtual ~IArchiveHandleStreamRuntime() = default;
    [[nodiscard]] virtual std::int32_t Seek(
        std::uint32_t handle, std::int32_t offset,
        std::int32_t origin) = 0;
    [[nodiscard]] virtual std::int32_t Position(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t Read(
        void* buffer, std::uint32_t size, std::uint32_t count,
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t Write(
        const void* buffer, std::uint32_t size, std::uint32_t count,
        std::uint32_t handle) = 0;
};

class ArchiveHandleStreamRuntimeStub final : public IArchiveHandleStreamRuntime {
public:
    [[nodiscard]] std::int32_t Seek(std::uint32_t, std::int32_t,
                                    std::int32_t) override { return 0; }
    [[nodiscard]] std::int32_t Position(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t Read(void*, std::uint32_t,
                                      std::uint32_t,
                                      std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Write(const void*, std::uint32_t,
                                       std::uint32_t,
                                       std::uint32_t) override { return 0u; }
};

class ArchiveHandleStream final {
public:
    static constexpr std::uint32_t kVtable = 0x8496ccu;
    explicit ArchiveHandleStream(IArchiveHandleStreamRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5ec860..0x5ec920: game-file handle implementation.
    ArchiveFileStreamState& Construct(ArchiveFileStreamState& state,
                                      std::uint32_t handle);
    [[nodiscard]] std::int32_t Size(const ArchiveFileStreamState& state);
    [[nodiscard]] std::int32_t SetPosition(
        const ArchiveFileStreamState& state, std::int32_t offset);
    [[nodiscard]] bool ReadElement(const ArchiveFileStreamState& state,
                                   void* buffer, std::uint32_t size);
    [[nodiscard]] bool WriteElement(const ArchiveFileStreamState& state,
                                    const void* buffer, std::uint32_t size);

private:
    IArchiveHandleStreamRuntime& runtime_;
};

}  // namespace pal4
