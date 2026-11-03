#pragma once
#include <cstdint>
#include <functional>
#include <string_view>
namespace pal4 {
class NativePrescriptionRecordCatalog;
class NativeInventorySession;
class NativeBaseObjectMemory;
class NativePlayerScriptRefreshRuntime;
struct PlayerManagerObjectState;
// Original590FB0 mutation primitive. The Smith UI must perform its original
// requirement/selection gates before calling; failure can consume materials.
class NativeCraftingApplication final {
public:
    using SoundPlayer=std::function<void(std::string_view,std::uint32_t)>;
    NativeCraftingApplication(const NativePrescriptionRecordCatalog&,NativeInventorySession&,
        PlayerManagerObjectState&,NativeBaseObjectMemory&,NativePlayerScriptRefreshRuntime&,SoundPlayer);
    std::int32_t Apply(std::uint32_t recipe,std::uint32_t selected_sequence,bool opened_from_system);
private:
    const NativePrescriptionRecordCatalog& recipes_;
    NativeInventorySession& inventory_;
    PlayerManagerObjectState& players_;
    NativeBaseObjectMemory& memory_;
    NativePlayerScriptRefreshRuntime& refresh_;
    SoundPlayer sound_;
};
}
