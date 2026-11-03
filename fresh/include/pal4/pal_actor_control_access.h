#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct PalActorControlAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IPalActorControlAccessRuntime {
public:
    virtual ~IPalActorControlAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) = 0;
    [[nodiscard]] virtual void* Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual void* Construct(void* storage) = 0;
    virtual void Destruct(void* object) = 0;
    [[nodiscard]] virtual std::uint32_t Free(void* object) = 0;
};

class PalActorControlAccessRuntimeStub : public IPalActorControlAccessRuntime {
public:
    std::uint32_t SingletonManager() override;
    std::uint32_t Register(std::uint32_t, std::string_view,
                           std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
    void* Allocate(std::uint32_t) override;
    void* Construct(void*) override;
    void Destruct(void*) override;
    std::uint32_t Free(void*) override;
};

class PalActorControlAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e4178u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e4174u;
    static constexpr std::uint32_t kSingletonId = 1014u;
    static constexpr std::uint32_t kFactoryAddress = 0x4235e0u;
    static constexpr std::uint32_t kDestroyAddress = 0x423640u;
    static constexpr std::uint32_t kObjectBytes = 0x464u;

    [[nodiscard]] static std::uint32_t Get(
        PalActorControlAccessState& state,
        IPalActorControlAccessRuntime& runtime);
    [[nodiscard]] static void* Create(IPalActorControlAccessRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        void* object, std::uint32_t null_result,
        IPalActorControlAccessRuntime& runtime);
};

}  // namespace pal4
