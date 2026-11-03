#include "pal4/game_data_parameter_field_names.h"

#include <array>
#include <span>
#include <stdexcept>

namespace pal4 {
namespace {

enum class Method { DirectAssign, ResetAssign, DirectAllocate, ResetAllocate };

struct NameSpec {
    std::string_view text;
    Method method;
};

constexpr std::array kAttack = {
    NameSpec{"PRA_NAME", Method::DirectAssign},
    NameSpec{"PRA_DESCRIPTION", Method::ResetAllocate},
    NameSpec{"PRA_PROPERTY", Method::ResetAllocate},
    NameSpec{"PRA_EF2", Method::ResetAllocate},
    NameSpec{"PRA_EF3", Method::ResetAllocate},
    NameSpec{"PRA_EF4", Method::ResetAssign},
};
constexpr std::array kProduct = {
    NameSpec{"PRA_PRODUCT_ID", Method::DirectAssign},
    NameSpec{"PRA_NEED_POTENTIAL", Method::DirectAssign},
    NameSpec{"PRA_NEED_MAGIC", Method::DirectAssign},
    NameSpec{"PRA_SKILL_ID", Method::ResetAllocate},
    NameSpec{"PRA_PRICE", Method::ResetAllocate},
};
constexpr std::array kProgression = {
    NameSpec{"PRI_MAX_HP", Method::ResetAllocate},
    NameSpec{"PRI_ADDED_RAGE", Method::ResetAllocate},
    NameSpec{"PRI_MAX_MP", Method::ResetAllocate},
    NameSpec{"PRI_WU", Method::ResetAllocate},
    NameSpec{"PRI_DEFENCE", Method::ResetAllocate},
    NameSpec{"PRI_SPEED", Method::ResetAllocate},
    NameSpec{"PRI_LUCKY", Method::ResetAssign},
    NameSpec{"PRI_MAGIC_ATTACK", Method::ResetAssign},
};
constexpr std::array kSpecial = {
    NameSpec{"PSA_ADDED_CRITICAL", Method::DirectAllocate},
    NameSpec{"PSA_FEND_OFF", Method::DirectAssign},
    NameSpec{"PSA_ADDED_HITTING", Method::DirectAllocate},
    NameSpec{"PSA_PAY_FOR_SPARE", Method::DirectAssign},
};
constexpr std::array kElements = {
    NameSpec{"PAA_PHYSICAL", Method::DirectAssign},
    NameSpec{"PAA_WATER", Method::ResetAllocate},
    NameSpec{"PAA_FIRE", Method::ResetAllocate},
    NameSpec{"PAA_THUNDER", Method::ResetAllocate},
    NameSpec{"PAA_AIR", Method::ResetAllocate},
    NameSpec{"PAA_EARTH", Method::ResetAssign},
};
constexpr std::array kMaterials = {
    NameSpec{"P_MAT_1", Method::DirectAllocate},
    NameSpec{"P_MAT_2", Method::DirectAssign},
    NameSpec{"P_MAT_3", Method::DirectAllocate},
    NameSpec{"P_MAT_4", Method::DirectAssign},
};

struct GroupSpec {
    std::uint32_t base;
    std::uint32_t destructor;
    std::span<const NameSpec> names;
};

GroupSpec Describe(ParameterFieldGroup group) {
    switch (group) {
    case ParameterFieldGroup::Attack:
        return {0x8eef90, 0x4e1200, kAttack};
    case ParameterFieldGroup::Product:
        return {0x8eef40, 0x4e13c0, kProduct};
    case ParameterFieldGroup::Progression:
        return {0x8eeec0, 0x4e16b0, kProgression};
    case ParameterFieldGroup::Special:
        return {0x8eee80, 0x4e1840, kSpecial};
    case ParameterFieldGroup::Elements:
        return {0x8eee20, 0x4e1a90, kElements};
    case ParameterFieldGroup::Materials:
        return {0x8eede0, 0x4e1c20, kMaterials};
    }
    throw std::invalid_argument("unknown parameter field group");
}

}  // namespace

std::optional<std::string_view> GameDataParameterFieldNames::NameAt(std::uint32_t address){
    for(const auto group:{ParameterFieldGroup::Attack,ParameterFieldGroup::Product,ParameterFieldGroup::Progression,
        ParameterFieldGroup::Special,ParameterFieldGroup::Elements,ParameterFieldGroup::Materials}){
        const auto spec=Describe(group);if(address<spec.base || (address-spec.base)%16u)continue;
        const auto i=(address-spec.base)/16u;if(i<spec.names.size())return spec.names[i].text;
    }return std::nullopt;
}

int GameDataParameterFieldNames::InitializeAndRegister(
    ParameterFieldGroup group, IGameDataParameterFieldNamesRuntime& runtime) {
    Initialize(group, runtime);
    return runtime.RegisterAtexit(Describe(group).destructor);
}

void GameDataParameterFieldNames::Initialize(
    ParameterFieldGroup group, IGameDataParameterFieldNamesRuntime& runtime) {
    const auto spec = Describe(group);
    const auto seed = runtime.InitialSeed();
    for (std::size_t i = 0; i < spec.names.size(); ++i) {
        const auto address = spec.base + static_cast<std::uint32_t>(i) * 16;
        const auto& field = spec.names[i];
        runtime.WriteByte(address, seed);
        if (field.method == Method::ResetAssign ||
            field.method == Method::ResetAllocate) {
            runtime.ResetText(address);
        } else {
            for (std::uint32_t offset : {4u, 8u, 12u})
                runtime.WriteWord(address + offset, 0);
        }
        if (field.method == Method::DirectAssign ||
            field.method == Method::ResetAssign)
            runtime.AssignText(address, field.text);
        else
            (void)runtime.AllocateText(address, field.text);
    }
}

void GameDataParameterFieldNames::Destroy(
    ParameterFieldGroup group, IGameDataParameterFieldNamesRuntime& runtime) {
    const auto spec = Describe(group);
    for (std::size_t i = spec.names.size(); i > 0; --i)
        runtime.DestroyText(spec.base + static_cast<std::uint32_t>(i - 1) * 16);
}

}  // namespace pal4
