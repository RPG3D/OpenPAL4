#pragma once

#include <cstdint>

namespace pal4 {

struct OperatingSystemVersion {
    std::uint32_t size = 0u;
    std::uint32_t major = 0u;
    std::uint32_t minor = 0u;
    std::uint32_t build = 0u;
    std::uint32_t platform = 0u;
    char service_pack[128]{};
};

static_assert(sizeof(OperatingSystemVersion) == 148u);

class IPlatformFunctionRuntime {
public:
    virtual ~IPlatformFunctionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t IsBadWritePointer(
        std::uint32_t address, std::uint32_t bytes) = 0;
    virtual void SetLastErrorEx(std::uint32_t error, std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t GetVersion(
        OperatingSystemVersion& version) = 0;
    [[nodiscard]] virtual std::uint32_t ExecuteRemoteFunction(
        std::uint32_t process_id, std::uint32_t count,
        std::uint32_t modules, std::uint32_t output) = 0;
    [[nodiscard]] virtual std::uint32_t EnumerateProcessModules(
        std::uint32_t process_id, std::uint32_t count,
        std::uint32_t modules, std::uint32_t output) = 0;
};

class PlatformFunctionDispatcher final {
public:
    [[nodiscard]] static std::uint32_t Dispatch(
        std::uint32_t process_id, std::uint32_t count,
        std::uint32_t modules, std::uint32_t output,
        IPlatformFunctionRuntime& runtime);
};

class OperatingSystemPlatformCache final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e0890u;
    static constexpr std::uint32_t kIsNtAddress = 0x8a1604u;

    OperatingSystemPlatformCache(std::uint32_t& guard, std::uint32_t& is_nt)
        : guard_(guard), is_nt_(is_nt) {}

    [[nodiscard]] std::uint32_t IsNt(IPlatformFunctionRuntime& runtime);

private:
    std::uint32_t& guard_;
    std::uint32_t& is_nt_;
};

}  // namespace pal4
