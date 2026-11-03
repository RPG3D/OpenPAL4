#pragma once

#include "pal4/actor_manager_string_method_property.h"

#include <cstdint>

namespace pal4 {

class ActorManagerPlayerUNameDescriptor final {
public:
    static constexpr std::uint32_t kDerivedVtable = 0x845268u;
    static constexpr std::uint32_t kBaseVtable = 0x842bd4u;

    static void Destroy(std::uint32_t descriptor,
                        IActorManagerStringMethodRuntime& runtime);
};

}  // namespace pal4
