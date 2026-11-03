#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IRenderWareHAnimRuntime {
public:
    using FrameVisitor = std::function<std::uint32_t(std::uint32_t)>;

    virtual ~IRenderWareHAnimRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetHierarchy(
        std::uint32_t frame) = 0;
    virtual void ForAllChildren(std::uint32_t frame,
                                const FrameVisitor& visitor) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void SetSkinHierarchy(std::uint32_t atomic,
                                  std::uint32_t hierarchy) = 0;
    [[nodiscard]] virtual std::uint32_t GetFrameId(
        std::uint32_t frame) = 0;
};

class RenderWareHAnimStub final : public IRenderWareHAnimRuntime {
public:
    [[nodiscard]] std::uint32_t GetHierarchy(std::uint32_t frame) override;
    void ForAllChildren(std::uint32_t frame,
                        const FrameVisitor& visitor) override;
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void SetSkinHierarchy(std::uint32_t atomic,
                          std::uint32_t hierarchy) override;
    [[nodiscard]] std::uint32_t GetFrameId(std::uint32_t frame) override;
};

class HAnimFrameWalker final {
public:
    [[nodiscard]] static std::uint32_t Visit(
        std::uint32_t frame, std::uint32_t& found,
        IRenderWareHAnimRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindInAtomicChildren(
        std::uint32_t atomic, IRenderWareHAnimRuntime& runtime);
    [[nodiscard]] static std::uint32_t AssignSkinHierarchy(
        std::uint32_t atomic, std::uint32_t hierarchy,
        IRenderWareHAnimRuntime& runtime);
    [[nodiscard]] static std::uint32_t CaptureFrameId(
        std::uint32_t frame, std::uint32_t& id,
        IRenderWareHAnimRuntime& runtime);
};

}  // namespace pal4
