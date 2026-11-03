#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

struct PalGameIVFactoryRegistration {
    std::uint32_t key;
    std::uint32_t factory;
};

class IPalGameIVObjectFactoryRuntime {
public:
    virtual ~IPalGameIVObjectFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EventVolumeRegistry() = 0;
    virtual void RegisterEventVolume(std::uint32_t registry,
                                     PalGameIVFactoryRegistration entry) = 0;
    [[nodiscard]] virtual std::uint32_t GameObjectRegistry() = 0;
    virtual void RegisterGameObject(std::uint32_t registry,
                                    PalGameIVFactoryRegistration entry) = 0;
    [[nodiscard]] virtual std::uint32_t StorageObjectRegistry() = 0;
    [[nodiscard]] virtual bool RegisterStorageObject(
        std::uint32_t registry, PalGameIVFactoryRegistration entry) = 0;
};

class PalGameIVObjectFactories final {
public:
    [[nodiscard]] static std::span<const PalGameIVFactoryRegistration>
        GameObjects();
    [[nodiscard]] static bool RegisterAll(
        IPalGameIVObjectFactoryRuntime& runtime);
};

}  // namespace pal4
