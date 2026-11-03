#pragma once

#include <cstdint>

namespace pal4 {

struct ActorDirectionVector {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class ITranslateActorTaskRuntime {
public:
    virtual ~ITranslateActorTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetPosition(std::uint32_t actor,
                             std::uint32_t task_position,
                             std::uint32_t mode) = 0;
};

class IActorPrepareStateRuntime {
public:
    virtual ~IActorPrepareStateRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual void SetPrepareState(std::uint32_t actor,std::uint32_t state)=0;
};
class IActorDirectionTaskRuntime : public ITranslateActorTaskRuntime,public IActorPrepareStateRuntime {
public:
    virtual ~IActorDirectionTaskRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) override=0;
    virtual void ConstructBase(std::uint32_t task) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual ActorDirectionVector ChildPosition(
        std::uint32_t actor) = 0;
    virtual void Normalize(ActorDirectionVector& vector) = 0;
    [[nodiscard]] virtual double AngleRadians(float cosine) = 0;
    virtual void SetFacing(std::uint32_t actor,
                           const ActorDirectionVector& rotation,
                           std::uint32_t mode) = 0;
    virtual void SetPrepareState(std::uint32_t actor,
                                 std::uint32_t state) = 0;
};

class IFaceTaskCreationRuntime : public IActorDirectionTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t task) = 0;
};

class TranslateActorTask final {
public:
    // IDA 0x55a7a0.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, ITranslateActorTaskRuntime& runtime);
};

class FaceTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845760u;
    static constexpr std::uint32_t kSize = 76u;

    // IDA 0x55a920-0x55a940.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t task, IActorDirectionTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IActorDirectionTaskRuntime& runtime);
    // IDA 0x5656e0: factory behind vtable 0x845940.
    [[nodiscard]] static std::uint32_t Create(
        IFaceTaskCreationRuntime& runtime);
};

class ChangeToPrepareActionTask final {
public:
    // IDA 0x55ad50.
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IActorPrepareStateRuntime& runtime);
};

}  // namespace pal4
