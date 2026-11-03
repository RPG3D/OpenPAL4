#pragma once
#include "pal4/native_dialog_properties.h"
#include <functional>
namespace pal4 {
// Borrows the owning Dialog registry and optional typed facet reader. Both must
// remain valid until every referencing camera/effect has stopped.
class NativeEffectDialogTargets final {
public:
    using Resolve=std::function<NativeDialogProperties&(std::uint32_t)>;
    explicit NativeEffectDialogTargets(Resolve);
    void BindFacetReader(NativeDialogProperties::Reader);
    std::array<float,3> ReadLive(std::uint32_t dialog,std::string_view name);
    std::array<float,3> ReadStored(std::uint32_t dialog,std::string_view name);
private:
    Resolve resolve_;
    NativeDialogProperties::Reader reader_;
    unsigned depth_{};
};
}
