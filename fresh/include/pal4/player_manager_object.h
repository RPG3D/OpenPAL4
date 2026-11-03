#pragma once

#include "pal4/base_object_lifecycle.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct PlayerManagerSlotState {
    BaseObjectState base{};
    std::array<std::uint32_t, (0xb14u - BaseObjectState::kSize) / 4u> tail{};
};
static_assert(sizeof(PlayerManagerSlotState) == 0xb14u);
static_assert(offsetof(PlayerManagerSlotState, base) == 0u);
static_assert(offsetof(PlayerManagerSlotState, tail) == BaseObjectState::kSize);

struct PlayerManagerObjectState {
    std::array<PlayerManagerSlotState, 4u> slots{};
    std::array<std::uint32_t, (0x2c64u - 4u * 0xb14u) / 4u> tail{};

    [[nodiscard]] std::uint32_t& Word(std::size_t index) noexcept {
        assert(index < 0x2c64u / sizeof(std::uint32_t));
        if (index >= 4u * (0xb14u / 4u))
            return tail[index - 4u * (0xb14u / 4u)];
        auto& slot = slots[index / (0xb14u / 4u)];
        index %= 0xb14u / 4u;
        if (index < BaseObjectState::kSize / 4u)
            return slot.base.words[index];
        return slot.tail[index - BaseObjectState::kSize / 4u];
    }
    [[nodiscard]] const std::uint32_t& Word(std::size_t index) const noexcept {
        return const_cast<PlayerManagerObjectState*>(this)->Word(index);
    }
    void Fill(std::uint32_t value) noexcept {
        for (auto& slot : slots) {
            slot.base.words.fill(value);
            slot.tail.fill(value);
        }
        tail.fill(value);
    }
};
static_assert(sizeof(PlayerManagerObjectState) == 0x2c64u);
static_assert(offsetof(PlayerManagerObjectState, slots) == 0u);
static_assert(offsetof(PlayerManagerObjectState, tail) == 4u * 0xb14u);

class IPlayerManagerVectorRuntime {
public:
    virtual ~IPlayerManagerVectorRuntime() = default;
    virtual void ConstructVector(std::uint32_t object,
        std::uint32_t element_bytes, std::uint32_t count,
        std::uint32_t constructor, std::uint32_t destructor) = 0;
    virtual void DestroyVector(std::uint32_t object,
        std::uint32_t element_bytes, std::uint32_t count,
        std::uint32_t destructor) = 0;
};

class PlayerManagerVectorRuntimeStub final : public IPlayerManagerVectorRuntime {
public:
    void ConstructVector(std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t) override;
    void DestroyVector(std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t) override;
};

class IPlayerManagerCallbackRuntime {
public:
    virtual ~IPlayerManagerCallbackRuntime() = default;
    virtual void Invoke(std::uint32_t callback, std::uint32_t slot,
                        std::uint32_t context) = 0;
};

class PlayerManagerCallbackRuntimeStub : public IPlayerManagerCallbackRuntime {
public:
    void Invoke(std::uint32_t, std::uint32_t, std::uint32_t) override;
};

class PlayerManagerObject final {
public:
    static constexpr std::uint32_t kElementBytes = 0xb14u;
    static constexpr std::uint32_t kElementCount = 4u;
    static constexpr std::uint32_t kConstructor = 0x43e6a0u;
    static constexpr std::uint32_t kDestructor = 0x43e890u;

    // IDA 0x441200.
    [[nodiscard]] static std::uint32_t Construct(
        PlayerManagerObjectState& state, std::uint32_t object,
        IPlayerManagerVectorRuntime& runtime);
    // IDA 0x441230.
    static void Destroy(std::uint32_t object,
        IPlayerManagerVectorRuntime& runtime);
    // IDA 0x441a00. Negative indexes are not rejected by the original code.
    [[nodiscard]] static std::uint32_t GetPlayerByIndex(
        std::uint32_t object, std::int32_t index) noexcept;
    // IDA 0x441a30.
    [[nodiscard]] static std::uint32_t GetActivePlayerCount(
        const PlayerManagerObjectState& state) noexcept;
    // IDA 0x441a50. The stored current index must identify one of four slots.
    [[nodiscard]] static std::uint32_t SetPlayerState(
        PlayerManagerObjectState& state, std::uint32_t value) noexcept;
    // IDA 0x442240. Invokes the callback for all four slots, including inactive.
    static void ForAllPlayers(std::uint32_t manager,
                              std::uint32_t callback,
                              std::uint32_t context,
                              IPlayerManagerCallbackRuntime& runtime);
};

}  // namespace pal4
