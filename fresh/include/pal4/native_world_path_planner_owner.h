#pragma once

#include "pal4/native_base_object_memory.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace pal4 {

class NativeWorldPathPlannerOwner final {
public:
    NativeWorldPathPlannerOwner();
    ~NativeWorldPathPlannerOwner();
    NativeWorldPathPlannerOwner(const NativeWorldPathPlannerOwner&) = delete;
    NativeWorldPathPlannerOwner& operator=(const NativeWorldPathPlannerOwner&) = delete;

    [[nodiscard]] std::uint32_t Construct();
    [[nodiscard]] bool LoadScene(const std::filesystem::path& game_root,
                                 std::string_view region,
                                 std::string_view scene);
    [[nodiscard]] bool SceneMemberMissing() const noexcept;
    // Borrowed grid image; invalidated by scene reload or Destroy. Exact name match.
    [[nodiscard]] std::uint32_t FindPath(std::string_view name) const;
    [[nodiscard]] std::uint32_t Address() const noexcept { return planner_; }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept;
    void Destroy();

private:
    class Runtime;
    NativeBaseObjectMemory memory_;
    std::unique_ptr<Runtime> runtime_;
    std::uint32_t planner_ = 0u;
};

}  // namespace pal4
