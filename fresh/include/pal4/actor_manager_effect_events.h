#pragma once

#include <cstdint>

#include "pal4/dname_node_event_record.h"

namespace pal4 {

class IActorManagerEffectEventRuntime : public IDNameNodeEventRuntime {
public:
    virtual ~IActorManagerEffectEventRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    virtual void DispatchWord(std::uint32_t router, std::uint32_t key,
                              std::uint32_t& value,
                              std::uint32_t mode) = 0;
    virtual void DispatchAddress(std::uint32_t router, std::uint32_t key,
                                 std::uint32_t payload,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryPayload() = 0;
    [[nodiscard]] virtual std::uint32_t ResetArgumentPair(
        std::uint32_t& first, std::uint32_t& second) = 0;
};

class ActorManagerEffectEvents final {
public:
    static constexpr std::uint32_t kCombatStartKey = 0x8b8b88u;
    static constexpr std::uint32_t kEffectEndKey = 0x8b8b64u;
    static constexpr std::uint32_t kEffectMessageKey = 0x8b8b60u;
    static constexpr std::uint32_t kEmptyText = 0x8dc780u;

    [[nodiscard]] static std::uint32_t HandleEffectEnd(
        std::uint32_t manager, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IActorManagerEffectEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleEffectMessage(
        std::uint32_t first, std::uint32_t second,
        std::uint32_t text, IActorManagerEffectEventRuntime& runtime);
};

}  // namespace pal4
