#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IPackageMessageBoxRuntime {
public:
    virtual ~IPackageMessageBoxRuntime() = default;
    [[nodiscard]] virtual std::uint32_t LastError() = 0;
    [[nodiscard]] virtual std::string SystemErrorMessage(
        std::uint32_t error, std::uint32_t language) = 0;
    [[nodiscard]] virtual int Show(const char* text, const char* caption,
                                   std::uint32_t type) = 0;
};

class PackageMessageBox final {
public:
    explicit PackageMessageBox(IPackageMessageBoxRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x794190.
    [[nodiscard]] int ShowFormatted(const char* format, ...);
    // IDA 0x7941C0.
    [[nodiscard]] int ShowSystemError(std::uint32_t type,
                                      const char* format, ...);

private:
    IPackageMessageBoxRuntime& runtime_;
};

}  // namespace pal4
