#include "pal4/pal_script_statics.h"

namespace pal4 {
namespace {

void NullSub82() {}

int RegisterThird(ICrtAtexitApi& api) {
    return api.Register(&NullSub82);
}

}  // namespace

int PalScriptStatics::Initialize(PalScriptStaticGuards& guards,
                                 ICrtAtexitApi& api) {
    guards.first = static_cast<std::uint8_t>(guards.first | 1u);
    (void)RegisterFirst(api);
    guards.second = static_cast<std::uint8_t>(guards.second | 1u);
    (void)RegisterSecond(api);
    guards.third = static_cast<std::uint8_t>(guards.third | 1u);
    return RegisterThird(api);
}

int PalScriptStatics::RegisterFirst(ICrtAtexitApi& api) {
    return api.Register(&NullSub82);
}

int PalScriptStatics::RegisterSecond(ICrtAtexitApi& api) {
    return api.Register(&NullSub82);
}

}  // namespace pal4
