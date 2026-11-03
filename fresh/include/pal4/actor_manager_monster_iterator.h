#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerMonsterIteratorRuntime {
public:
    virtual ~IActorManagerMonsterIteratorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerMonsterIterator final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x844fc4u;
    static constexpr std::uint32_t kDerivedVtable = 0x844f98u;

    static void MarkBase(std::uint32_t iterator,
                         IActorManagerMonsterIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source, IActorManagerMonsterIteratorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t iterator, std::uint8_t flags,
        IActorManagerMonsterIteratorRuntime& runtime);
};

}  // namespace pal4
