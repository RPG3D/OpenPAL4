#include "pal4/combat_event_registry_callbacks.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<CombatEventRegistryCallbacks::Callback,
                     CombatEventRegistryCallbacks::kCount> kCallbacks{{
    &CombatEventRegistryCallbacks::NullSub4,
    &CombatEventRegistryCallbacks::NullSub5,
    &CombatEventRegistryCallbacks::NullSub6,
    &CombatEventRegistryCallbacks::NullSub7,
    &CombatEventRegistryCallbacks::NullSub8,
    &CombatEventRegistryCallbacks::NullSub9,
    &CombatEventRegistryCallbacks::NullSub10,
    &CombatEventRegistryCallbacks::NullSub11,
    &CombatEventRegistryCallbacks::NullSub12,
    &CombatEventRegistryCallbacks::NullSub13,
    &CombatEventRegistryCallbacks::NullSub14,
    &CombatEventRegistryCallbacks::NullSub15,
    &CombatEventRegistryCallbacks::NullSub16,
    &CombatEventRegistryCallbacks::NullSub17,
    &CombatEventRegistryCallbacks::NullSub18,
    &CombatEventRegistryCallbacks::NullSub19,
    &CombatEventRegistryCallbacks::NullSub20,
    &CombatEventRegistryCallbacks::NullSub21,
    &CombatEventRegistryCallbacks::NullSub22,
    &CombatEventRegistryCallbacks::NullSub23,
    &CombatEventRegistryCallbacks::NullSub24,
    &CombatEventRegistryCallbacks::NullSub25,
    &CombatEventRegistryCallbacks::NullSub26,
    &CombatEventRegistryCallbacks::NullSub27,
    &CombatEventRegistryCallbacks::NullSub28,
    &CombatEventRegistryCallbacks::NullSub29,
    &CombatEventRegistryCallbacks::NullSub30,
    &CombatEventRegistryCallbacks::NullSub31,
    &CombatEventRegistryCallbacks::NullSub32,
    &CombatEventRegistryCallbacks::NullSub33,
    &CombatEventRegistryCallbacks::NullSub34,
    &CombatEventRegistryCallbacks::NullSub35,
    &CombatEventRegistryCallbacks::NullSub36,
    &CombatEventRegistryCallbacks::NullSub37,
    &CombatEventRegistryCallbacks::NullSub38,
    &CombatEventRegistryCallbacks::NullSub39,
    &CombatEventRegistryCallbacks::NullSub40,
    &CombatEventRegistryCallbacks::NullSub41,
    &CombatEventRegistryCallbacks::NullSub42,
    &CombatEventRegistryCallbacks::NullSub43,
    &CombatEventRegistryCallbacks::NullSub44,
    &CombatEventRegistryCallbacks::NullSub45,
    &CombatEventRegistryCallbacks::NullSub46,
    &CombatEventRegistryCallbacks::NullSub47,
    &CombatEventRegistryCallbacks::NullSub48,
    &CombatEventRegistryCallbacks::NullSub49,
    &CombatEventRegistryCallbacks::NullSub50,
    &CombatEventRegistryCallbacks::NullSub51,
    &CombatEventRegistryCallbacks::NullSub52,
    &CombatEventRegistryCallbacks::NullSub53,
    &CombatEventRegistryCallbacks::NullSub54,
    &CombatEventRegistryCallbacks::NullSub55,
    &CombatEventRegistryCallbacks::NullSub56,
    &CombatEventRegistryCallbacks::NullSub57,
    &CombatEventRegistryCallbacks::NullSub58,
    &CombatEventRegistryCallbacks::NullSub59,
    &CombatEventRegistryCallbacks::NullSub60,
    &CombatEventRegistryCallbacks::NullSub61,
    &CombatEventRegistryCallbacks::NullSub62,
    &CombatEventRegistryCallbacks::NullSub63,
    &CombatEventRegistryCallbacks::NullSub64,
    &CombatEventRegistryCallbacks::NullSub65,
    &CombatEventRegistryCallbacks::NullSub66,
    &CombatEventRegistryCallbacks::NullSub67,
    &CombatEventRegistryCallbacks::NullSub68,
    &CombatEventRegistryCallbacks::NullSub69,
    &CombatEventRegistryCallbacks::NullSub70,
    &CombatEventRegistryCallbacks::NullSub71,
    &CombatEventRegistryCallbacks::NullSub72,
    &CombatEventRegistryCallbacks::NullSub73,
    &CombatEventRegistryCallbacks::NullSub74,
    &CombatEventRegistryCallbacks::NullSub75,
    &CombatEventRegistryCallbacks::NullSub76,
    &CombatEventRegistryCallbacks::NullSub77,
    &CombatEventRegistryCallbacks::NullSub78,
    &CombatEventRegistryCallbacks::NullSub79,
    &CombatEventRegistryCallbacks::NullSub80,
}};

}  // namespace

CombatEventRegistryCallbacks::Callback CombatEventRegistryCallbacks::Resolve(
    std::uint32_t native_address) {
    if (native_address < kFirstAddress) return nullptr;
    const auto offset = native_address - kFirstAddress;
    if (offset % 16u != 0u || offset / 16u >= kCount) return nullptr;
    return kCallbacks[offset / 16u];
}

void CombatEventRegistryCallbacks::NullSub4() {}
void CombatEventRegistryCallbacks::NullSub5() {}
void CombatEventRegistryCallbacks::NullSub6() {}
void CombatEventRegistryCallbacks::NullSub7() {}
void CombatEventRegistryCallbacks::NullSub8() {}
void CombatEventRegistryCallbacks::NullSub9() {}
void CombatEventRegistryCallbacks::NullSub10() {}
void CombatEventRegistryCallbacks::NullSub11() {}
void CombatEventRegistryCallbacks::NullSub12() {}
void CombatEventRegistryCallbacks::NullSub13() {}
void CombatEventRegistryCallbacks::NullSub14() {}
void CombatEventRegistryCallbacks::NullSub15() {}
void CombatEventRegistryCallbacks::NullSub16() {}
void CombatEventRegistryCallbacks::NullSub17() {}
void CombatEventRegistryCallbacks::NullSub18() {}
void CombatEventRegistryCallbacks::NullSub19() {}
void CombatEventRegistryCallbacks::NullSub20() {}
void CombatEventRegistryCallbacks::NullSub21() {}
void CombatEventRegistryCallbacks::NullSub22() {}
void CombatEventRegistryCallbacks::NullSub23() {}
void CombatEventRegistryCallbacks::NullSub24() {}
void CombatEventRegistryCallbacks::NullSub25() {}
void CombatEventRegistryCallbacks::NullSub26() {}
void CombatEventRegistryCallbacks::NullSub27() {}
void CombatEventRegistryCallbacks::NullSub28() {}
void CombatEventRegistryCallbacks::NullSub29() {}
void CombatEventRegistryCallbacks::NullSub30() {}
void CombatEventRegistryCallbacks::NullSub31() {}
void CombatEventRegistryCallbacks::NullSub32() {}
void CombatEventRegistryCallbacks::NullSub33() {}
void CombatEventRegistryCallbacks::NullSub34() {}
void CombatEventRegistryCallbacks::NullSub35() {}
void CombatEventRegistryCallbacks::NullSub36() {}
void CombatEventRegistryCallbacks::NullSub37() {}
void CombatEventRegistryCallbacks::NullSub38() {}
void CombatEventRegistryCallbacks::NullSub39() {}
void CombatEventRegistryCallbacks::NullSub40() {}
void CombatEventRegistryCallbacks::NullSub41() {}
void CombatEventRegistryCallbacks::NullSub42() {}
void CombatEventRegistryCallbacks::NullSub43() {}
void CombatEventRegistryCallbacks::NullSub44() {}
void CombatEventRegistryCallbacks::NullSub45() {}
void CombatEventRegistryCallbacks::NullSub46() {}
void CombatEventRegistryCallbacks::NullSub47() {}
void CombatEventRegistryCallbacks::NullSub48() {}
void CombatEventRegistryCallbacks::NullSub49() {}
void CombatEventRegistryCallbacks::NullSub50() {}
void CombatEventRegistryCallbacks::NullSub51() {}
void CombatEventRegistryCallbacks::NullSub52() {}
void CombatEventRegistryCallbacks::NullSub53() {}
void CombatEventRegistryCallbacks::NullSub54() {}
void CombatEventRegistryCallbacks::NullSub55() {}
void CombatEventRegistryCallbacks::NullSub56() {}
void CombatEventRegistryCallbacks::NullSub57() {}
void CombatEventRegistryCallbacks::NullSub58() {}
void CombatEventRegistryCallbacks::NullSub59() {}
void CombatEventRegistryCallbacks::NullSub60() {}
void CombatEventRegistryCallbacks::NullSub61() {}
void CombatEventRegistryCallbacks::NullSub62() {}
void CombatEventRegistryCallbacks::NullSub63() {}
void CombatEventRegistryCallbacks::NullSub64() {}
void CombatEventRegistryCallbacks::NullSub65() {}
void CombatEventRegistryCallbacks::NullSub66() {}
void CombatEventRegistryCallbacks::NullSub67() {}
void CombatEventRegistryCallbacks::NullSub68() {}
void CombatEventRegistryCallbacks::NullSub69() {}
void CombatEventRegistryCallbacks::NullSub70() {}
void CombatEventRegistryCallbacks::NullSub71() {}
void CombatEventRegistryCallbacks::NullSub72() {}
void CombatEventRegistryCallbacks::NullSub73() {}
void CombatEventRegistryCallbacks::NullSub74() {}
void CombatEventRegistryCallbacks::NullSub75() {}
void CombatEventRegistryCallbacks::NullSub76() {}
void CombatEventRegistryCallbacks::NullSub77() {}
void CombatEventRegistryCallbacks::NullSub78() {}
void CombatEventRegistryCallbacks::NullSub79() {}
void CombatEventRegistryCallbacks::NullSub80() {}

}  // namespace pal4
