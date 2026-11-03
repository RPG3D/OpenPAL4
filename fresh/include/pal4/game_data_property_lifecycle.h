#pragma once

#include <cstdint>

namespace pal4 {

class IGameDataPropertyLifecycleRuntime {
public:
    virtual ~IGameDataPropertyLifecycleRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void FreeMemory(std::uint32_t object) = 0;
};

class GameDataPropertyLifecycle final {
public:
    // IDA 0x4c89f0: set the property base vtable.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IGameDataPropertyLifecycleRuntime& runtime);
    // IDA 0x4cbe80: set the numeric-property base vtable.
    [[nodiscard]] static std::uint32_t ConstructNumericBase(
        std::uint32_t object, IGameDataPropertyLifecycleRuntime& runtime);
    // IDA 0x4c8a00: base destructor then optional delete.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint8_t flags,
        IGameDataPropertyLifecycleRuntime& runtime);
};

}  // namespace pal4
