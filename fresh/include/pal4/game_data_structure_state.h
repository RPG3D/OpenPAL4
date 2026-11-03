#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataStructureStateRuntime {
public:
    virtual ~IGameDataStructureStateRuntime() = default;
    virtual void InitializeOuterBase(std::uint32_t object) = 0;
    virtual void InitializeInnerBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class GameDataStructureState final {
public:
    // IDA 0x4c8850: outer base then state initializer.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGameDataStructureStateRuntime& runtime);
    // IDA 0x4c8870: clear fields and establish default scalar values.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, IGameDataStructureStateRuntime& runtime);
};

}  // namespace pal4
