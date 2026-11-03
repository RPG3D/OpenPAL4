#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
namespace pal4 {
// Modern public stream boundary for57DE60/57E1E0/57E610. Original game text,
// threshold, startup buffering and file policy; no emulated VC6 stream image.
class NativeCombatEventLogger final {
public:
    NativeCombatEventLogger();
    ~NativeCombatEventLogger();
    NativeCombatEventLogger(const NativeCombatEventLogger&) = delete;
    NativeCombatEventLogger& operator=(const NativeCombatEventLogger&) = delete;
    void Construct();
    void SetLogFile(const std::filesystem::path&,std::uint32_t mode);
    void SetThreshold(std::uint32_t level);
    void Log(std::uint32_t level,std::string_view message);
    void Destroy();
    [[nodiscard]] bool IsOpen() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
