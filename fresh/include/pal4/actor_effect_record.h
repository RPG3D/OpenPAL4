#pragma once

#include <cstdint>

namespace pal4 {

class IActorEffectRecordRuntime {
public:
    virtual ~IActorEffectRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ResetHandle(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ActorEffectRecord final {
public:
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t destination, std::uint32_t source,
        IActorEffectRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t record, std::uint8_t flags,
        IActorEffectRecordRuntime& runtime);
};

}  // namespace pal4
