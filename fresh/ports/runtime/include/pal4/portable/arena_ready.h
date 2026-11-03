#pragma once
#include "pal4/portable/controller.h"
#include "pal4/portable/player.h"
#include "pal4/game_arena_ready_handler.h"
#include <iosfwd>
namespace pal4::portable {
class IArenaReadyServices {
public:
    virtual ~IArenaReadyServices()=default;
    virtual void InitializePetStoredData()=0;
    virtual void UpdateGrid(std::array<float,3> position,std::array<float,3> rotation)=0;
    virtual std::span<const pal4::ArenaReadyHintRecord> HintRecords()=0;
    virtual std::uint32_t CreateHintWindow()=0;
    virtual void ShowHint(std::uint32_t window,std::string_view markup,float x,float y,float duration,std::uint32_t first,std::uint32_t second)=0;
    virtual std::uint32_t Milliseconds()=0;
};
// State owned by this arena. Original 66 writes ready and three clocks only
// after its controller/pet/minimap calls have succeeded.
class ArenaReady {
public:
    // restoring comes from arena lifecycle state, not from a script request.
    // New scene loads leave it false; save-state loading is not implemented here.
    ArenaReady(std::string world,std::string scene,PlayerRoster& players,ActorController& controller,
        IControllerResetServices& input,IArenaReadyServices& services,bool restoring=false);
    ~ArenaReady();
    ArenaReady(const ArenaReady&)=delete;
    ArenaReady& operator=(const ArenaReady&)=delete;
    void Run();
    void Restore();
    void WriteReport(std::ostream& output) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
