#include "pal4/skill_parameter_text_registry.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, SkillParameterTextRegistry::kCount>
    kLiterals = {"SKILL_PARAM_1", "SKILL_PARAM_2", "SKILL_PARAM_3",
                 "SKILL_PARAM_4", "SKILL_PARAM_5", "SKILL_PARAM_6",
                 "SKILL_PARAM_7", "SKILL_PARAM_8", "SKILL_PARAM_9"};

constexpr std::uint32_t AddressAt(std::uint32_t index) {
    return SkillParameterTextRegistry::kFirstAddress +
           index * SkillParameterTextRegistry::kTextBytes;
}

}  // namespace

int SkillParameterTextRegistry::InitializeAndRegisterCleanup(
    ISkillParameterTextRuntime& runtime) {
    static_cast<void>(Initialize(runtime));
    return runtime.RegisterCleanup();
}

int SkillParameterTextRegistry::Initialize(
    ISkillParameterTextRuntime& runtime) {
    for (std::uint32_t index = 0; index < 6; ++index) {
        const auto address = AddressAt(index);
        runtime.InitializeText(address);
        static_cast<void>(runtime.CopyLiteral(address, kLiterals[index]));
    }
    int result = 0;
    for (std::uint32_t index = 6; index < kCount; ++index) {
        const auto address = AddressAt(index);
        runtime.InitializeText(address);
        result = runtime.SetExceptionMessage(address, kLiterals[index]);
    }
    return result;
}

void SkillParameterTextRegistry::Destroy(
    ISkillParameterTextRuntime& runtime) {
    for (std::uint32_t index = kCount; index != 0;)
        runtime.DestroyText(AddressAt(--index));
}

}  // namespace pal4
