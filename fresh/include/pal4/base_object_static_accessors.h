#pragma once

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class BaseObjectStaticAccessors final {
public:
    // IDA 0x444440.
    static void SetDefaultVtable(BaseObjectState& state);
    // IDA 0x444450 and 0x444460.
    [[nodiscard]] static const char* HoldString();
    [[nodiscard]] static const char* RpeString();
};

}  // namespace pal4
