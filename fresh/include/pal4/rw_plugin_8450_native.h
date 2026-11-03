#pragma once
#include "pal4/rw_plugin_8450.h"

namespace pal4 {
// Single RenderWare engine/thread, matching the game's global plugin state.
// Attach after RwEngineInit/RpWorldPluginAttach and before RwEngineOpen.
// Storage and callback context have process lifetime; renderer services are separate.
class NativeRwPlugin8450Lifecycle final : public IRwPlugin8450Lifecycle {
public:
    [[nodiscard]] static bool Attach();
    [[nodiscard]] static const RwPlugin8450State& State();
private:
    NativeRwPlugin8450Lifecycle() = default;
    static NativeRwPlugin8450Lifecycle& Instance();
    RwPlugin8450State state_;
    std::int32_t RegisterEngine(std::uint32_t,std::uint32_t,RwPlugin8450Callback,RwPlugin8450Callback) override;
    std::int32_t RegisterAtomic(std::uint32_t,std::uint32_t,RwPlugin8450Callback,RwPlugin8450Callback,RwPlugin8450Callback) override;
    std::int32_t RegisterAtomicStream(std::uint32_t,RwPlugin8450Callback,RwPlugin8450Callback,RwPlugin8450Callback) override;
    std::uint32_t EngineBase() override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
    static void* EngineConstruct(void*,int,int);
    static void* EngineDestruct(void*,int,int);
    static void* AtomicConstruct(void*,int,int);
    static void* AtomicDestruct(void*,int,int);
    static void* AtomicCopy(void*,const void*,int,int);
};
}
