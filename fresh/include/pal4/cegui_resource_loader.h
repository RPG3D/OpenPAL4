#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>

namespace pal4 {

class CeguiRawDataContainer final {
public:
    static constexpr std::uint32_t kBytes = 12u;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const noexcept;
    void WriteDword(std::uint32_t offset, std::uint32_t value) noexcept;

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class CeguiInvalidRequest final : public std::runtime_error {
public:
    enum class Reason { EmptyName, MissingFile };
    explicit CeguiInvalidRequest(Reason reason);
    [[nodiscard]] Reason GetReason() const noexcept;

private:
    Reason reason_;
};

class ICeguiResourceLoaderRuntime {
public:
    virtual ~ICeguiResourceLoaderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t StringStorage(
        std::uint32_t string_address) = 0;
    [[nodiscard]] virtual bool EqualsEmptyString(
        std::uint32_t string_address) = 0;
    [[nodiscard]] virtual bool UsePackageFiles() = 0;
    [[nodiscard]] virtual std::uint32_t BuildUtf8(
        std::uint32_t string_address) = 0;
    virtual void LoadUiScriptPath(std::uint32_t utf8_address,
                                  char* output, std::uint32_t capacity) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeManager() = 0;
    [[nodiscard]] virtual std::uint32_t PackageOpen(
        std::uint32_t manager, const char* path) = 0;
    [[nodiscard]] virtual std::uint32_t PackageSize(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t CreateBuffer(
        std::uint32_t provider, std::uint32_t size) = 0;
    [[nodiscard]] virtual bool PackageRead(
        std::uint32_t manager, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t handle) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void PackageClose(std::uint32_t manager,
                              std::uint32_t handle) = 0;
    [[nodiscard]] virtual bool LocalOpen(std::uint32_t utf8_address) = 0;
    [[nodiscard]] virtual std::uint32_t LocalSize() = 0;
    virtual void LocalRead(std::uint32_t buffer,
                           std::uint32_t size) = 0;
    [[nodiscard]] virtual bool LocalClose() = 0;
};

class CeguiResourceLoaderRuntimeStub : public ICeguiResourceLoaderRuntime {
public:
    [[nodiscard]] std::uint32_t StringStorage(
        std::uint32_t string_address) override;
    [[nodiscard]] bool EqualsEmptyString(
        std::uint32_t string_address) override;
    [[nodiscard]] bool UsePackageFiles() override;
    [[nodiscard]] std::uint32_t BuildUtf8(
        std::uint32_t string_address) override;
    void LoadUiScriptPath(std::uint32_t utf8_address,
                          char* output, std::uint32_t capacity) override;
    [[nodiscard]] std::uint32_t InitializeManager() override;
    [[nodiscard]] std::uint32_t PackageOpen(
        std::uint32_t manager, const char* path) override;
    [[nodiscard]] std::uint32_t PackageSize(
        std::uint32_t handle) override;
    [[nodiscard]] std::uint32_t CreateBuffer(
        std::uint32_t provider, std::uint32_t size) override;
    [[nodiscard]] bool PackageRead(
        std::uint32_t manager, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t handle) override;
    void Free(std::uint32_t address) override;
    void PackageClose(std::uint32_t manager,
                      std::uint32_t handle) override;
    [[nodiscard]] bool LocalOpen(std::uint32_t utf8_address) override;
    [[nodiscard]] std::uint32_t LocalSize() override;
    void LocalRead(std::uint32_t buffer,
                   std::uint32_t size) override;
    [[nodiscard]] bool LocalClose() override;
};

class CeguiResourceLoader final {
public:
    static constexpr std::uint32_t kManagerBase = 0x500990u;

    // 0x416740.
    static void Load(std::uint32_t provider,
                     std::uint32_t name_address,
                     CeguiRawDataContainer& output,
                     std::uint32_t unused,
                     ICeguiResourceLoaderRuntime& runtime);
};

}  // namespace pal4
