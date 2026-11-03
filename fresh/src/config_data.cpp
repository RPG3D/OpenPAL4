#include "pal4/config_data.h"

namespace pal4 {

ConfigDataState& ConfigData::Initialize(ConfigDataState& state) {
    state.words.fill(0u);
    return state;
}

}  // namespace pal4
