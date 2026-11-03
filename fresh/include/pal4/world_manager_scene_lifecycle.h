#pragma once

#include <cstdint>

namespace pal4 {

class IWorldManagerSceneRuntime {
public:
    virtual ~IWorldManagerSceneRuntime() = default;
    virtual void ConstructBaseWorld(std::uint32_t object) = 0;
    virtual void DestroyBaseWorld(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class WorldManagerSceneLifecycle final {
public:
    static constexpr std::uint32_t kVtable = 0x845308u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IWorldManagerSceneRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeletingDestructor(
        std::uint32_t object, std::uint32_t flags,
        IWorldManagerSceneRuntime& runtime);
    static void Destroy(std::uint32_t object,
                        IWorldManagerSceneRuntime& runtime);
};

}  // namespace pal4
