#include "pal4/portable/combat_runtime_owner.h"

#include <memory>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<pal4::portable::CombatRuntimeOwner>);

int main() {
    std::unique_ptr<pal4::portable::CombatRuntimeOwner> owner;
    return owner ? 1 : 0;
}
