#pragma once
#include <cstdint>
namespace pal4 {
class ICombatSystemStartReadyRuntime {
public:
    virtual ~ICombatSystemStartReadyRuntime()=default;
    virtual bool HasPlayers()=0;
    virtual void InstallInputCallback()=0;
    virtual void PublishStateOne()=0;
    virtual void DispatchPrepare(std::int32_t start_type)=0;
    virtual void ResetAudioPosition()=0;
};
class CombatSystemStartReady final {
public:
    //5756D0 tail: nonempty first Player roster -> callback -> state1 -> event
    // -> actual listener origin reset. Exceptions retain original partial commit.
    static bool Commit(std::int32_t start_type,ICombatSystemStartReadyRuntime&);
};
}
