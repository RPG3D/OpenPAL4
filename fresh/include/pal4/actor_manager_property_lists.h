#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPropertyListsRuntime {
public:
    virtual ~IActorManagerPropertyListsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class ActorManagerPropertyLists final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, IActorManagerPropertyListsRuntime& runtime);
};

}  // namespace pal4
