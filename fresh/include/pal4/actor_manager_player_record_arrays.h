#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerRecordArrayRuntime {
public:
    virtual ~IActorManagerPlayerRecordArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorManagerPlayerRecordArrays final {
public:
    [[nodiscard]] static std::uint32_t AssignPairs(
        std::uint32_t destination, std::uint32_t source,
        IActorManagerPlayerRecordArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t AssignTriples(
        std::uint32_t destination, std::uint32_t source,
        IActorManagerPlayerRecordArrayRuntime& runtime);
};

}  // namespace pal4
