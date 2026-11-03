#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerStateReadRuntime {
public:
    virtual ~IActorManagerPlayerStateReadRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot52(
        std::uint32_t actor, std::uint32_t mode) = 0;
};
class IActorManagerPlayerStateUpdateRuntime : public virtual IActorManagerPlayerStateReadRuntime {
public:
    [[nodiscard]] virtual std::uint32_t UpdateStateAndProcess(
        std::uint32_t actor, std::uint32_t state,
        std::uint32_t saved_value) = 0;
};

class IActorManagerPlayerStateLearningRuntime : public virtual IActorManagerPlayerStateReadRuntime {
public:
    virtual void BuildMatchingRecords(std::uint32_t collection,
                                      std::uint32_t output,
                                      std::uint32_t state,
                                      std::uint32_t saved_value) = 0;
};
class IActorManagerPlayerStateRecordRuntime : public IActorManagerPlayerStateUpdateRuntime,
                                            public IActorManagerPlayerStateLearningRuntime {};

class ActorManagerPlayerStateRecords final {
public:
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t actor, IActorManagerPlayerStateUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t BuildMatching(
        std::uint32_t actor, std::uint32_t output,
        IActorManagerPlayerStateLearningRuntime& runtime);
};

}  // namespace pal4
