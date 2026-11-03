#include "pal4/dialog_object.h"

namespace pal4 {

DialogTaskFactoryAdapterState& DialogTaskFactoryAdapter::Construct(
    DialogTaskFactoryAdapterState& state,
    const DialogTaskFactoryAdapterState& source) {
    state.words[0u] = kVtable;
    state.words[1u] = source.words[1u];
    return state;
}

}  // namespace pal4
