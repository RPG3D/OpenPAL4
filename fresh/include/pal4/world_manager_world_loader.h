#pragma once

#include "pal4/world_manager_scene_initializer.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// High-level game ordering can borrow an already integrated native scene
// loader without inventing the legacy scene/package/pipeline interfaces.
class IWorldManagerWorldLoadServices {
public:
    virtual ~IWorldManagerWorldLoadServices() = default;
    virtual void TouchSkillSystem() = 0;
    virtual void CompactAtomicList(std::uint32_t list) = 0;
    virtual std::string_view StoredText(std::uint32_t descriptor) = 0;
    virtual void AssignText(std::uint32_t descriptor,std::string_view text) = 0;
    virtual void InitializeScene(std::uint32_t world,std::string_view region,std::string_view scene) = 0;
    virtual std::uint32_t CurrentCameraObject() = 0;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) = 0;
    virtual std::uint32_t SetWorldCamera(std::uint32_t world,std::uint32_t camera) = 0;
};
class IWorldManagerWorldLoadRuntime : public IWorldManagerSceneInitializationRuntime,
                                     public IWorldManagerWorldLoadServices {
public:
    void InitializeScene(std::uint32_t world,std::string_view region,std::string_view scene) override;
    virtual void TouchSkillSystem() = 0;
    virtual void CompactAtomicList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::string_view StoredText(
        std::uint32_t descriptor) = 0;
    virtual void AssignText(std::uint32_t descriptor,
                            std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentCameraObject() = 0;
    [[nodiscard]] virtual std::uint32_t SetWorldCamera(
        std::uint32_t world, std::uint32_t camera) = 0;
};

class WorldManagerWorldLoader final {
public:
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t manager, std::string_view region,
        std::string_view scene, IWorldManagerWorldLoadServices& runtime);
};

}  // namespace pal4
