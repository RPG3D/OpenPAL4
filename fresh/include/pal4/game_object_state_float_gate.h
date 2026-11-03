#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

namespace pal4 {

class GameObjectStateFloatGate final {
public:
    // IDA 0x43b9d0: ordered comparison of helper floats +0xc4 and +0xc0.
    [[nodiscard]] static bool Check(
        const BehaviorPathEndpointState& helper);
};

}  // namespace pal4
