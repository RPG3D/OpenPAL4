#include "pal4/game_data_role_field_names.h"

#include <array>

namespace pal4 {
namespace {

struct NameSpec {
    std::string_view text;
    bool allocate;
};

constexpr std::array kNames = {
    NameSpec{"RA_NAME", true},
    NameSpec{"RA_ICON", false},
    NameSpec{"RA_MODEL", true},
    NameSpec{"RA_TEXTURE", false},
};
constexpr std::uint32_t kBase = 0x8ef1c8;
constexpr std::uint32_t kDestructor = 0x4e7840;

}  // namespace

std::optional<std::string_view> GameDataRoleFieldNames::NameAt(
    const std::uint32_t address) {
    if (address < kBase || (address - kBase) % 16u != 0u)
        return std::nullopt;
    const auto index = (address - kBase) / 16u;
    if (index >= kNames.size()) return std::nullopt;
    return kNames[index].text;
}

int GameDataRoleFieldNames::InitializeAndRegister(
    IGameDataRoleFieldNamesRuntime& runtime) {
    Initialize(runtime);
    return runtime.RegisterAtexit(kDestructor);
}

void GameDataRoleFieldNames::Initialize(
    IGameDataRoleFieldNamesRuntime& runtime) {
    const auto seed = runtime.InitialSeed();
    for (std::uint32_t i = 0; i < kNames.size(); ++i) {
        const auto address = kBase + i * 16;
        runtime.WriteByte(address, seed);
        for (std::uint32_t offset : {4u, 8u, 12u})
            runtime.WriteWord(address + offset, 0);
        if (kNames[i].allocate)
            (void)runtime.AllocateText(address, kNames[i].text);
        else
            runtime.AssignText(address, kNames[i].text);
    }
}

void GameDataRoleFieldNames::Destroy(
    IGameDataRoleFieldNamesRuntime& runtime) {
    for (std::uint32_t i = static_cast<std::uint32_t>(kNames.size());
         i > 0; --i)
        runtime.DestroyText(kBase + (i - 1) * 16);
}

}  // namespace pal4
