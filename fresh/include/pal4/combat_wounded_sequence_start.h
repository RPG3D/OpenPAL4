#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct WoundedSoundArgument {
    std::uint32_t value = 0u;
    std::uint32_t temporary_text = 0u;
};

class IWoundedSequenceStartRuntime {
public:
    virtual ~IWoundedSequenceStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorVirtual168(
        std::uint32_t actor) = 0;
    virtual void CheckActorStateGroup(std::uint32_t actor) = 0;
    virtual void SetActorState(std::uint32_t actor,
                               std::uint32_t state,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual WoundedSoundArgument ActorVirtual456(
        std::uint32_t actor, std::uint32_t mode) = 0;
    virtual void PlaySoundEffect(std::uint32_t argument,
                                 std::uint32_t mode) = 0;
    virtual void ReleaseSoundText(std::uint32_t temporary_text,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        std::uint32_t factory_vtable) = 0;
    [[nodiscard]] virtual std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) = 0;
    virtual void AssignText(std::uint32_t header,
                            std::uint32_t source,
                            std::uint32_t start,
                            std::uint32_t count) = 0;
};

}  // namespace pal4
