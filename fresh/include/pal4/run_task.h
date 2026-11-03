#pragma once

#include <cstdint>

namespace pal4 {

struct RunTaskPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class IRunTaskRuntime {
public:
    virtual ~IRunTaskRuntime() = default;
    virtual void ConstructBase(std::uint32_t task) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual RunTaskPosition ChildPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual double CalculateMagnitude(
        const RunTaskPosition& delta) = 0;
    virtual void Normalize(RunTaskPosition& delta) = 0;
    virtual void NotifyRun(std::uint32_t actor,
                           std::uint32_t payload, std::uint32_t mode) = 0;
    [[nodiscard]] virtual int SetChildPosition(
        std::uint32_t actor, const RunTaskPosition& position,
        std::uint32_t mode) = 0;
};

class IRunTaskCreationRuntime : public IRunTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t task) = 0;
};

class RunTask final {
public:
    static constexpr std::uint32_t kVtable = 0x845740u;
    static constexpr std::uint32_t kRunPayload = 0x8fa37cu;
    static constexpr std::uint32_t kSize = 88u;

    // IDA 0x55a4b0-0x55a580.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t task, IRunTaskRuntime& runtime);
    // IDA 0x565740: factory behind vtable 0x845944.
    [[nodiscard]] static std::uint32_t Create(
        IRunTaskCreationRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetTarget(
        std::uint32_t task, std::uint32_t actor,
        std::uint32_t source_position, IRunTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IRunTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdatePosition(
        std::uint32_t task, float step, IRunTaskRuntime& runtime);
};

}  // namespace pal4
