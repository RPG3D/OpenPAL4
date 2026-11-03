#include "pal4/game_data_est_registry.h"

#include <array>
#include <string_view>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 32> kFieldNames{
    "KEY", "BASE", "PRIZE_S", "PRIZE_1", "PRIZE_2", "PRIZE_3",
    "EST_PROTECT", "EST_JOINT_ATK", "EST_CRITICAL", "EST_MISSED",
    "EST_FEND_OFF", "EST_REACT_ATK", "EST_CAST_MAGIC", "EST_STUNT",
    "EST_USE_PROPERTY", "EST_REVIVE", "EST_FLEE_FAIL", "EST_ROLE_DEAD",
    "EST_UNATTACKED", "EST_HP_IN_MAX", "EST_HP_IN_DANGER",
    "EST_TAKE_ACTION", "EST_MONSTER_DIDNT_ACT", "EST_MONSTER_FLEE",
    "EST_MONSTER_DEAD", "EST_LEVEL_UPPER", "EST_LEVEL_LOWER",
    "EST_PHYSICAL_ATK", "EST_KNOCK_OUT", "EST_WIN_AT_THE_FIRST_BOUT",
    "EST_KO_BY_ONE_ROLE", "EST_ALL_SURVIVE"};

}  // namespace

const std::vector<GameDataFieldDescriptor>& GameDataEstRegistry::Initialize(
    IGameDataEstRegistryRuntime& runtime) {
    if (!globals_initialized_) {
        globals_initialized_ = true;
        runtime.InitializeGlobalStorage(kCollectionAddress);
        runtime.RegisterGlobalCleanup(kCleanupCallback);
    }
    if (entries_initialized_) return fields_;
    entries_initialized_ = true;

    for (std::uint32_t index = 0; index < kFieldNames.size(); ++index)
        runtime.AppendField(fields_,
                            {index * 4, std::string(kFieldNames[index]),
                             GameDataFieldKind::numeric, 0x843368, 24, 0, 0});
    runtime.Sort(fields_);
    return fields_;
}

void GameDataEstRegistry::Cleanup(IGameDataEstRegistryRuntime& runtime) {
    runtime.CleanupGlobalStorage(kCollectionAddress);
    fields_.clear();
}

std::uint8_t GameDataEstRegistry::Process(
    std::uint32_t collection, IGameDataEstRegistryRuntime& runtime) {
    return runtime.ProcessCollection(collection, Initialize(runtime));
}

}  // namespace pal4
