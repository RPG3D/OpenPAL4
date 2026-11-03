#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IJointAttackRuntime {
public:
    virtual ~IJointAttackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool StateThree(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t Value7(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float GameDBFloat(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t CreateIterator(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool NotStateThree(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorMode(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ActorProperty(
        std::uint32_t address) = 0;
    virtual void EmitRespond(std::string_view event_name,
                             std::uint32_t responder,
                             std::uint32_t target,
                             std::uint32_t mode) = 0;
};

class JointAttackTask final {
public:
    // IDA 0x558290, 0x5582a0 and 0x5584a0.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IJointAttackRuntime& runtime);
    static void ProcessInternal(std::uint32_t task,
                                IJointAttackRuntime& runtime);
    [[nodiscard]] static bool IsAttackable(
        std::uint32_t actor, IJointAttackRuntime& runtime);
};

}  // namespace pal4
