#pragma once

namespace pal4 {
struct PlayerManagerObjectState;
class NativeBaseObjectMemory;
class NativePlayerSlotCopyRuntime;
class NativeGameSaveArchiveReader;
class IPlayerManagerOperationsRuntime;
class IPlayerManagerSlotCollectionRuntime;
class IPlayerManagerSlotScriptRuntime;
class IScriptProcessRuntime;
class IGameResourceManagerRuntime;

// Required existing game services, borrowed from the persistent host.
struct NativePlayerManagerLoadServices {
    IPlayerManagerOperationsRuntime& prepare;
    IPlayerManagerSlotCollectionRuntime& collections;
    IPlayerManagerSlotScriptRuntime& refresh;
    IScriptProcessRuntime& scripts;
    IGameResourceManagerRuntime& transforms;
};
class NativePlayerManagerLoadRuntime final {
public:
    // Continuous P_MGR section, including preparation and all four players.
    [[nodiscard]] static bool Deserialize(PlayerManagerObjectState& manager,
        NativeBaseObjectMemory& memory, NativePlayerSlotCopyRuntime& text_owner,
        NativeGameSaveArchiveReader& archive, NativePlayerManagerLoadServices services);
};
}
