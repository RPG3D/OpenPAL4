#pragma once

#include <cstdint>

namespace pal4 {

class IPalGameIVComponentDestructorRuntime {
public:
    virtual ~IPalGameIVComponentDestructorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ResetEffectHandle(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyXmlNode(
        std::uint32_t node) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class PalGameIVComponentDestructors final {
public:
    [[nodiscard]] static std::uint32_t DestructScene(
        std::uint32_t object, IPalGameIVComponentDestructorRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestructCombat(
        std::uint32_t object, IPalGameIVComponentDestructorRuntime& runtime);
    static void DestructBase(
        std::uint32_t object, IPalGameIVComponentDestructorRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteScene(
        std::uint32_t object, std::uint8_t flags,
        IPalGameIVComponentDestructorRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteCombat(
        std::uint32_t object, std::uint8_t flags,
        IPalGameIVComponentDestructorRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteBase(
        std::uint32_t object, std::uint8_t flags,
        IPalGameIVComponentDestructorRuntime& runtime);
};

}  // namespace pal4
