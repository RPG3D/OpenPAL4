#pragma once

#include <cstdint>

namespace pal4 {

class IActorControlNamedMemberRuntime {
public:
    virtual ~IActorControlNamedMemberRuntime() = default;
    virtual void ResetBaseVtable(std::uint32_t object);
    virtual void Free(std::uint32_t object);
    virtual void CallFirstMember(std::uint32_t member);
    virtual void ResetMemberDisplay(std::uint32_t member);
};

struct PalActorControlNamedMemberPrefix {
    std::uint32_t vtable{};
    std::uint32_t member{};
};
static_assert(sizeof(PalActorControlNamedMemberPrefix) == 8u);

class PalActorControlNamedMember final {
public:
    static constexpr std::uint32_t kFirstVtable = 0x842660u;
    static constexpr std::uint32_t kSecondVtable = 0x842654u;
    // IDA 0x427560, slot +4 in the first vtable.
    [[nodiscard]] static const char* FirstTag();
    // IDA 0x427570, shared deleting destructor in both vtables.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint32_t flags,
        IActorControlNamedMemberRuntime& runtime);
    // IDA 0x427590, slot +4 in the second vtable.
    [[nodiscard]] static const char* SecondTag();
    [[nodiscard]] static std::uint32_t DispatchFirst(
        const PalActorControlNamedMemberPrefix& object,
        std::uint32_t unused_first, std::uint32_t unused_second,
        IActorControlNamedMemberRuntime& runtime);
    [[nodiscard]] static std::uint32_t DispatchSecond(
        const PalActorControlNamedMemberPrefix& object,
        std::uint32_t unused_first, std::uint32_t unused_second,
        IActorControlNamedMemberRuntime& runtime);
};

}  // namespace pal4
