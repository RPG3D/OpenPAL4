#pragma once

#include <cstdint>

namespace pal4 {

class IRpHAnimHierarchyRuntime;

using RpHAnimFrameCallback = std::uint32_t (*)(
    std::uint32_t frame, std::uint32_t& found,
    IRpHAnimHierarchyRuntime& runtime);

class IRpHAnimHierarchyRuntime {
public:
    virtual ~IRpHAnimHierarchyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameHierarchy(
        std::uint32_t frame) = 0;
    virtual void ForAllChildren(std::uint32_t frame,
                                RpHAnimFrameCallback callback,
                                std::uint32_t& found) = 0;
    [[nodiscard]] virtual std::uint32_t ClumpFrame(
        std::uint32_t clump) = 0;
    virtual void ForAllAtomicsGetId(std::uint32_t clump,
                                    std::uint32_t& found) = 0;
};

class RpHAnimHierarchyRuntimeStub final : public IRpHAnimHierarchyRuntime {
public:
    std::uint32_t FrameHierarchy(std::uint32_t) override { return 0; }
    void ForAllChildren(std::uint32_t, RpHAnimFrameCallback,
                        std::uint32_t&) override {}
    std::uint32_t ClumpFrame(std::uint32_t) override { return 0; }
    void ForAllAtomicsGetId(std::uint32_t,
                            std::uint32_t&) override {}
};

class RpHAnimHierarchy final {
public:
    [[nodiscard]] static std::uint32_t FindInFrame(
        std::uint32_t frame, std::uint32_t& found,
        IRpHAnimHierarchyRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindInClump(
        std::uint32_t clump, IRpHAnimHierarchyRuntime& runtime);
    [[nodiscard]] static bool HasHierarchy(
        std::uint32_t clump, IRpHAnimHierarchyRuntime& runtime);
    [[nodiscard]] static std::uint32_t* SetFlags(
        std::uint32_t& flags, std::uint32_t mask);
};

}  // namespace pal4
