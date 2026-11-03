#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IWorldManagerStageEventRuntime {
public:
    virtual ~IWorldManagerStageEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t DialogData(
        std::uint32_t event_args) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void ActivateActor(std::uint32_t actor,
                               std::uint32_t camera_argument,
                               std::uint32_t actor_data) = 0;
    virtual void LeaveActor(std::uint32_t actor,
                            const std::array<std::uint32_t, 3>& values,
                            std::uint32_t mode) = 0;
};

class WorldManagerStageEvents final {
public:
    // WorldManager's existing embedded scene starts at+116.
    static void JoinScene(std::uint32_t scene,std::uint32_t event_args,
                          IWorldManagerStageEventRuntime& runtime);
    static void JoinWrapper(std::uint32_t manager,
                            std::uint32_t event_args,
                            IWorldManagerStageEventRuntime& runtime);
    static void Join(std::uint32_t manager, std::uint32_t event_args,
                     IWorldManagerStageEventRuntime& runtime);
    static void LeaveWrapper(std::uint32_t manager,
                             std::uint32_t event_args,
                             IWorldManagerStageEventRuntime& runtime);
    static void Leave(std::uint32_t manager, std::uint32_t event_args,
                      IWorldManagerStageEventRuntime& runtime);
};

}  // namespace pal4
