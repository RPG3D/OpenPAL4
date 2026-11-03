#pragma once

#include "pal4/effect_script_vm.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

// IDA 0x53e3a0-0x53e4a0 uses the distinct VM slot at 0x8f94c4.
class MatFxScriptVm final {
public:
    static constexpr std::uint32_t kNativeVmSlot = 0x8f94c4u;

    [[nodiscard]] std::uint32_t ExecuteFile(
        std::string_view path, IEffectScriptVmRuntime& runtime);
    [[nodiscard]] std::uint32_t Release(IEffectScriptVmRuntime& runtime);
    [[nodiscard]] std::uint32_t GetState(
        IEffectScriptVmRuntime& runtime) const;
    [[nodiscard]] std::uint32_t GetProperty288(
        IEffectScriptVmRuntime& runtime) const;
    [[nodiscard]] std::uint32_t Vm() const { return vm_.Vm(); }

private:
    EffectScriptVm vm_;
};

}  // namespace pal4
