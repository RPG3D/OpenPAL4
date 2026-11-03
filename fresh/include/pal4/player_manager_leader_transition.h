#pragma once

#include <cstdint>
#include <vector>

namespace pal4 {

class IPlayerManagerPriorityRuntime {
public:
    virtual ~IPlayerManagerPriorityRuntime()=default;
    virtual std::int32_t GetPriority(std::uint32_t script)=0;
};
class IPlayerManagerLeaderRuntime : public IPlayerManagerPriorityRuntime {
public:
    virtual ~IPlayerManagerLeaderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetStateFlag(std::uint32_t slot, std::uint32_t value) = 0;
    virtual void SetAction(std::uint32_t action_component,
                           std::uint32_t action_name) = 0;
    virtual void CopyTransform(std::uint32_t slot, std::uint32_t virtual_slot,
                               std::uint32_t source, std::uint32_t option) = 0;
    virtual void SetPlayerState(std::uint32_t manager, std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t Arena() = 0;
    virtual void OnRemove(std::uint32_t arena, std::uint32_t slot) = 0;
    virtual void OnAdd(std::uint32_t arena, std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void BindActor(std::uint32_t control, std::uint32_t slot) = 0;
    virtual void SortActive(std::vector<std::uint32_t>& slots,
                            std::uint32_t comparator) = 0;
    [[nodiscard]] virtual std::int32_t GetPriority(std::uint32_t script) = 0;
    virtual void SetPriority(std::uint32_t script, std::int32_t value) = 0;
};

class PlayerManagerLeaderRuntimeStub : public IPlayerManagerLeaderRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void SetStateFlag(std::uint32_t, std::uint32_t) override;
    void SetAction(std::uint32_t, std::uint32_t) override;
    void CopyTransform(std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t) override;
    void SetPlayerState(std::uint32_t, std::uint32_t) override;
    std::uint32_t Arena() override;
    void OnRemove(std::uint32_t, std::uint32_t) override;
    void OnAdd(std::uint32_t, std::uint32_t) override;
    std::uint32_t ActorControl() override;
    void BindActor(std::uint32_t, std::uint32_t) override;
    void SortActive(std::vector<std::uint32_t>&, std::uint32_t) override;
    std::int32_t GetPriority(std::uint32_t) override;
    void SetPriority(std::uint32_t, std::int32_t) override;
};

class PlayerManagerLeaderTransition final {
public:
    static constexpr std::uint32_t kCurrentIndexOffset = 0x2c50u;
    static constexpr std::uint32_t kMembershipOffset = 0xb08u;
    static constexpr std::uint32_t kPlayerStateOffset = 0xb0cu;
    static constexpr std::uint32_t kScriptOffset = 0x654u;
    static constexpr std::uint32_t kActionOffset = 0x170u;
    static constexpr std::uint32_t kActionName = 0x8a6884u;
    static constexpr std::uint32_t kComparator = 0x4419d0u;

    // IDA 0x441730. The caller supplies a valid player index (0..3).
    static void Switch(std::uint32_t manager, std::uint32_t requested_index,
                       IPlayerManagerLeaderRuntime& runtime);
    // IDA 0x4419d0. The right getter is called before the left getter.
    [[nodiscard]] static bool ComparePlayerStates(
        std::uint32_t left_slot, std::uint32_t right_slot,
        IPlayerManagerPriorityRuntime& runtime);
};

}  // namespace pal4
