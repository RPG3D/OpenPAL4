#pragma once

#include <cstdint>

namespace pal4 {

struct CeguiTreeGlobals {
    static constexpr std::uint32_t kReferenceCountAddress = 0x8e40c4u;
    static constexpr std::uint32_t kNilAddress = 0x8e40c8u;

    std::uint32_t reference_count = 0u;
    std::uint32_t nil = 0u;
};

class ICeguiLifecycleRuntime {
public:
    virtual ~ICeguiLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void CleanupNodesRecursive(std::uint32_t map_base,
                                       std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceCursor(
        std::uint32_t cursor) = 0;
    virtual void DestroyNode(std::uint32_t map_base,
                             std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetSystemSingletonPtr() = 0;
    [[nodiscard]] virtual std::uint32_t GetImageManager() = 0;
    virtual void DestroyAllImages(std::uint32_t manager) = 0;
    virtual void InvokeDeletingDestructor(std::uint32_t object,
                                          std::uint32_t flags) = 0;
    virtual void DestructSystem(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t GetSystemSingleton() = 0;
    [[nodiscard]] virtual std::uint32_t InjectTimePulse(
        std::uint32_t system, float delta) = 0;
    virtual void RenderGui(std::uint32_t system) = 0;
};

class CeguiLifecycleStub final : public ICeguiLifecycleRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    void CleanupNodesRecursive(std::uint32_t map_base,
                               std::uint32_t root) override;
    [[nodiscard]] std::uint32_t AdvanceCursor(
        std::uint32_t cursor) override;
    void DestroyNode(std::uint32_t map_base,
                     std::uint32_t node) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t GetSystemSingletonPtr() override;
    [[nodiscard]] std::uint32_t GetImageManager() override;
    void DestroyAllImages(std::uint32_t manager) override;
    void InvokeDeletingDestructor(std::uint32_t object,
                                  std::uint32_t flags) override;
    void DestructSystem(std::uint32_t system) override;
    [[nodiscard]] std::uint32_t GetSystemSingleton() override;
    [[nodiscard]] std::uint32_t InjectTimePulse(
        std::uint32_t system, float delta) override;
    void RenderGui(std::uint32_t system) override;
};

class CeguiMapTreeLifecycle final {
public:
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t map_base, CeguiTreeGlobals& globals,
        ICeguiLifecycleRuntime& runtime);
};

class CeguiSystemLifecycle final {
public:
    [[nodiscard]] static std::uint32_t DestroySystem(
        std::uint32_t system, std::uint32_t flags,
        ICeguiLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyUi(
        std::uint32_t owner, CeguiTreeGlobals& globals,
        ICeguiLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t TimePulse(
        float delta, ICeguiLifecycleRuntime& runtime);
    static void Render(ICeguiLifecycleRuntime& runtime);
};

}  // namespace pal4
