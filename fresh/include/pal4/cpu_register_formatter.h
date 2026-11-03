#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class ICpuRegisterRuntime {
public:
    virtual ~ICpuRegisterRuntime() = default;
    [[nodiscard]] virtual bool IsBadReadPointer(
        std::uint32_t address, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
};

class CpuRegisterFormatter final {
public:
    static constexpr std::uint32_t kOriginalBufferAddress = 0x8df878u;

    [[nodiscard]] const char* Generate(
        std::uint32_t exception_pointers, ICpuRegisterRuntime& runtime);
    [[nodiscard]] const std::string& Report() const { return report_; }

private:
    std::string report_;
};

}  // namespace pal4
