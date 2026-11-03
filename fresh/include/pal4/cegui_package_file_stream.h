#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class CeguiPackageFileStreamState final {
public:
    static constexpr std::uint32_t kBytes = 8u;
    static constexpr std::uint32_t kVtable = 0x842148u;

    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const noexcept;
    void WriteDword(std::uint32_t offset, std::uint32_t value) noexcept;

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class ICeguiPackageFileRuntime {
public:
    virtual ~ICeguiPackageFileRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BaseVtable() = 0;
    virtual void LoadUiScriptPath(std::uint32_t resource_address,
                                  char* output, std::uint32_t capacity) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeManager() = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        std::uint32_t manager, const char* path) = 0;
    [[nodiscard]] virtual std::int32_t SeekFile(
        std::uint32_t manager, std::uint32_t handle,
        std::int32_t offset, std::uint32_t origin) = 0;
    virtual void CloseFile(std::uint32_t manager,
                           std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool ReadFile(
        std::uint32_t manager, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::int32_t ReadPosition(
        std::uint32_t handle) = 0;
};

class CeguiPackageFileRuntimeStub : public ICeguiPackageFileRuntime {
public:
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t BaseVtable() override;
    void LoadUiScriptPath(std::uint32_t resource_address,
                          char* output, std::uint32_t capacity) override;
    [[nodiscard]] std::uint32_t InitializeManager() override;
    [[nodiscard]] std::uint32_t OpenFile(
        std::uint32_t manager, const char* path) override;
    [[nodiscard]] std::int32_t SeekFile(
        std::uint32_t manager, std::uint32_t handle,
        std::int32_t offset, std::uint32_t origin) override;
    void CloseFile(std::uint32_t manager,
                   std::uint32_t handle) override;
    [[nodiscard]] bool ReadFile(
        std::uint32_t manager, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t handle) override;
    [[nodiscard]] std::int32_t ReadPosition(
        std::uint32_t handle) override;
};

class CeguiPackageFileStream final {
public:
    static constexpr std::uint32_t kManagerBase = 0x500990u;

    // 0x416510, 0x416520, 0x416540.
    [[nodiscard]] static CeguiPackageFileStreamState& Construct(
        CeguiPackageFileStreamState& state) noexcept;
    [[nodiscard]] static CeguiPackageFileStreamState& Destroy(
        CeguiPackageFileStreamState& state, std::uint8_t flags,
        std::uint32_t object_address, ICeguiPackageFileRuntime& runtime);
    [[nodiscard]] static std::uint32_t Deinitialize(
        CeguiPackageFileStreamState& state,
        ICeguiPackageFileRuntime& runtime);

    // 0x416590 through 0x4166f0.
    [[nodiscard]] static bool Open(
        CeguiPackageFileStreamState& state,
        std::uint32_t resource_address, std::uint32_t unused,
        ICeguiPackageFileRuntime& runtime);
    [[nodiscard]] static std::int32_t Close(
        CeguiPackageFileStreamState& state,
        ICeguiPackageFileRuntime& runtime);
    [[nodiscard]] static std::uint32_t Read(
        const CeguiPackageFileStreamState& state,
        std::uint32_t buffer, std::uint32_t size,
        std::uint32_t count, ICeguiPackageFileRuntime& runtime);
    [[nodiscard]] static std::int32_t Seek(
        const CeguiPackageFileStreamState& state,
        std::int32_t offset, std::uint32_t origin,
        ICeguiPackageFileRuntime& runtime);
    [[nodiscard]] static std::int32_t Position(
        const CeguiPackageFileStreamState& state,
        ICeguiPackageFileRuntime& runtime);
};

}  // namespace pal4
