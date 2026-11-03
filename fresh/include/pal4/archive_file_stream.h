#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ArchiveFileStreamState {
    std::uint32_t vtable{};
    std::uint32_t file{};
};
static_assert(sizeof(ArchiveFileStreamState) == 8u);
static_assert(offsetof(ArchiveFileStreamState, file) == 4u);

class IArchiveFileStreamRuntime {
public:
    virtual ~IArchiveFileStreamRuntime() = default;
    [[nodiscard]] virtual std::int32_t Seek(
        std::uint32_t file, std::int32_t offset, std::int32_t origin) = 0;
    [[nodiscard]] virtual std::int32_t Tell(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t Read(
        void* buffer, std::uint32_t size, std::uint32_t count,
        std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t Write(
        const void* buffer, std::uint32_t size, std::uint32_t count,
        std::uint32_t file) = 0;
};

class ArchiveFileStreamRuntimeStub final : public IArchiveFileStreamRuntime {
public:
    [[nodiscard]] std::int32_t Seek(std::uint32_t, std::int32_t,
                                    std::int32_t) override { return 0; }
    [[nodiscard]] std::int32_t Tell(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t Read(void*, std::uint32_t,
                                      std::uint32_t,
                                      std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Write(const void*, std::uint32_t,
                                       std::uint32_t,
                                       std::uint32_t) override { return 0u; }
};

class ArchiveFileStream final {
public:
    static constexpr std::uint32_t kVtable = 0x8496b4u;
    explicit ArchiveFileStream(IArchiveFileStreamRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ec610..0x5ec6e0.
    ArchiveFileStreamState& Construct(ArchiveFileStreamState& state,
                                      std::uint32_t file);
    [[nodiscard]] std::int32_t Size(const ArchiveFileStreamState& state);
    [[nodiscard]] std::int32_t Position(const ArchiveFileStreamState& state);
    [[nodiscard]] std::int32_t SetPosition(
        const ArchiveFileStreamState& state, std::int32_t offset);
    [[nodiscard]] bool ReadElement(const ArchiveFileStreamState& state,
                                   void* buffer, std::uint32_t size);
    [[nodiscard]] bool WriteElement(const ArchiveFileStreamState& state,
                                    const void* buffer, std::uint32_t size);

private:
    IArchiveFileStreamRuntime& runtime_;
};

}  // namespace pal4
