#pragma once

#include "pal4/native_player_basic_field_schema.h"
#include "pal4/script_binding_catalog.h"

#include <vector>

namespace pal4 {

class NativePlayerFieldSchema final {
public:
    [[nodiscard]] static std::vector<NativePlayerBasicField> Build();
    [[nodiscard]] static std::vector<ScriptBindingSpec> ScriptBindings();
};

}  // namespace pal4
