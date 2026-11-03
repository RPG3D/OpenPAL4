#pragma once
#include "pal4/native_skill_system_services.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
namespace pal4 {
class NativePlayerCombatEvents;
class RandomNumberGenerator;
// Owns actual58A050/58A100/58A170 initialization and58A0B0 cleanup. The logger
// uses host streams outside the reserved legacy prefix. Game dispatch words
// remain native x86 memory; callback addresses are evidence tokens only.
// Borrows the game-wide RNG and guarded property event registry.
class NativeSkillSystemOwner final : public INativeSkillSystemServices {
public:
    NativeSkillSystemOwner(RandomNumberGenerator&,NativePlayerCombatEvents&);
    ~NativeSkillSystemOwner() override;
    NativeSkillSystemOwner(const NativeSkillSystemOwner&) = delete;
    NativeSkillSystemOwner& operator=(const NativeSkillSystemOwner&) = delete;
    // An explicit clock sample supports reproducible comparison; ordinary
    // callers retain the actual timeGetTime sample at original58A170.
    std::uint32_t Construct(const std::filesystem::path& output_directory,
        std::optional<std::uint32_t> initialization_clock = std::nullopt);
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept override;
    [[nodiscard]] NativePlayerCombatEvents& PropertyEvents() noexcept override;
    [[nodiscard]] RandomNumberGenerator& Random() noexcept override;
    [[nodiscard]] std::size_t OwnedAllocations() const noexcept;
    [[nodiscard]] std::uint32_t FindCallback(std::int32_t id);
    [[nodiscard]] std::uint32_t Seed() const noexcept;
    void Log(std::uint32_t level,std::string_view message);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
