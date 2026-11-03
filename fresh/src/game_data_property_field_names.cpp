#include "pal4/game_data_property_field_names.h"

#include <array>
#include <span>

namespace pal4 {
namespace {

enum class Method { DirectAssign, ResetAssign, ResetAllocate };

struct NameSpec {
    std::string_view text;
    Method method;
};

constexpr std::array kCore = {
    NameSpec{"PA_NAME", Method::ResetAllocate},
    NameSpec{"PA_DESCRIPTION", Method::ResetAllocate},
    NameSpec{"PA_PROPERTY", Method::ResetAllocate},
    NameSpec{"PA_ICON", Method::ResetAllocate},
    NameSpec{"PA_MODEL", Method::ResetAllocate},
    NameSpec{"PA_TEXTURE", Method::ResetAllocate},
    NameSpec{"PA_ATTACHED_EFFECT", Method::ResetAssign},
};

constexpr std::array kExtra = {
    NameSpec{"PA_PRICE", Method::DirectAssign},
    NameSpec{"PA_ATTACHED_SKILL", Method::DirectAssign},
    NameSpec{"PRIZE_LEVEL", Method::DirectAssign},
    NameSpec{"PA_ATK_TYPE", Method::ResetAllocate},
    NameSpec{"CANDROP", Method::ResetAllocate},
};

struct GroupSpec {
    std::uint32_t base;
    std::uint32_t destructor;
    std::span<const NameSpec> names;
};

GroupSpec Describe(PropertyFieldGroup group) {
    if (group == PropertyFieldGroup::Core)
        return {0x8ef080, 0x4e4c10, kCore};
    return {0x8ef030, 0x4e4dd0, kExtra};
}

}  // namespace

int GameDataPropertyFieldNames::InitializeAndRegister(
    PropertyFieldGroup group, IGameDataPropertyFieldNamesRuntime& runtime) {
    Initialize(group, runtime);
    return runtime.RegisterAtexit(Describe(group).destructor);
}

void GameDataPropertyFieldNames::Initialize(
    PropertyFieldGroup group, IGameDataPropertyFieldNamesRuntime& runtime) {
    const auto spec = Describe(group);
    const auto seed = runtime.InitialSeed();
    for (std::size_t i = 0; i < spec.names.size(); ++i) {
        const auto address = spec.base + static_cast<std::uint32_t>(i) * 16;
        const auto& field = spec.names[i];
        runtime.WriteByte(address, seed);
        if (field.method == Method::DirectAssign) {
            for (std::uint32_t offset : {4u, 8u, 12u})
                runtime.WriteWord(address + offset, 0);
        } else {
            runtime.ResetText(address);
        }
        if (field.method == Method::ResetAllocate)
            (void)runtime.AllocateText(address, field.text);
        else
            runtime.AssignText(address, field.text);
    }
}

void GameDataPropertyFieldNames::Destroy(
    PropertyFieldGroup group, IGameDataPropertyFieldNamesRuntime& runtime) {
    const auto spec = Describe(group);
    for (std::size_t i = spec.names.size(); i > 0; --i)
        runtime.DestroyText(spec.base +
                            static_cast<std::uint32_t>(i - 1) * 16);
}

}  // namespace pal4
