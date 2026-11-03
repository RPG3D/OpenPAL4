#pragma once
#include "pal4/behavior_helper_configuration.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeBehaviorGridAssociationRuntime;
// Win32 boundary. Actual images/association payloads remain native; all setup
// algorithms and temporary vector contexts live in the shared typed core.
class NativeBehaviorHelperConfiguration final {
public:
    using Random=std::function<std::uint32_t()>;
    NativeBehaviorHelperConfiguration(NativeBaseObjectMemory&,NativeBehaviorGridAssociationRuntime&,Random);
    ~NativeBehaviorHelperConfiguration();
    [[nodiscard]] std::uint32_t Configure(std::uint32_t helper,std::uint32_t wrapper,std::uint32_t path);
    [[nodiscard]] static BehaviorVector3 ReadVector(const BehaviorPathEndpointState& helper,std::uint32_t slot);
    static void SetVector(BehaviorPathEndpointState& helper,std::uint32_t slot,const BehaviorVector3& value);
private:
    class Bindings;std::unique_ptr<Bindings> bindings_;
    std::unique_ptr<BehaviorHelperConfiguration> core_;
};
}
