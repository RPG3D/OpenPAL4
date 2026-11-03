#include "pal4/combat_action_console_window_visibility.h"

namespace pal4 {
namespace {

bool HasAvailableMagic(
    ICombatActionConsoleWindowVisibilityRuntime& runtime) {
    const auto iterator = runtime.AcquireActorIterator(
        runtime.ActorManager() + 0x20u);
    bool found = false;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto actor = runtime.IteratorCurrent(iterator);
            if (actor != 0u && runtime.ActorHasMagic(actor + 0x398u)) {
                found = true;
                break;
            }
            runtime.IteratorNext(iterator);
        }
    } catch (...) {
        if (iterator != 0u) runtime.ReleaseIterator(iterator, 1u);
        throw;
    }
    if (iterator != 0u) runtime.ReleaseIterator(iterator, 1u);
    return found;
}

}  // namespace

std::uint32_t CombatActionConsoleWindowVisibility::Update(
    const std::uint32_t window, const std::uint32_t actor_context,
    const std::uint8_t visible,
    ICombatActionConsoleWindowVisibilityRuntime& runtime,
    IEventHandlerArrayRuntime& array_runtime) {
    class Adapter final:public ICombatActionConsoleDisplayRuntime {
        ICombatActionConsoleWindowVisibilityRuntime& r;IEventHandlerArrayRuntime& a;
    public:
        Adapter(ICombatActionConsoleWindowVisibilityRuntime& r,IEventHandlerArrayRuntime& a):r(r),a(a){}
        std::uint32_t ReadWord(std::uint32_t p)override{return r.ReadWord(p);}
        void WriteWord(std::uint32_t p,std::uint32_t v)override{r.WriteWord(p,v);}
        void WriteByte(std::uint32_t p,std::uint8_t v)override{r.WriteByte(p,v);}
        std::uint8_t ActorProperty(std::uint32_t c,std::uint32_t i)override{return r.ActorProperty(c,i);}
        bool HasAvailableMagic()override{return pal4::HasAvailableMagic(r);}
        void SetEnabled(std::uint32_t w,std::uint8_t v)override{r.SetEnabled(w,v);}
        void RequestRedraw(std::uint32_t w)override{r.RequestRedraw(w);}
        bool IsDisabled(std::uint32_t w,bool b)override{return r.IsDisabled(w,b);}
        void SetVisible(std::uint32_t w,std::uint8_t v)override{r.SetVisible(w,v);}
        std::uint32_t SetInputActivity(std::uint32_t h,bool v)override{
            const auto p=r.FrameManager()+0x134u;
            return v?EventHandlerArray::Insert(p,h,a):EventHandlerArray::Remove(p,h,a);
        }
    } adapter(runtime,array_runtime);
    return Update(window,actor_context,visible,adapter);
}

std::uint32_t CombatActionConsoleWindowVisibility::Update(
    std::uint32_t window,std::uint32_t actor_context,std::uint8_t visible,
    ICombatActionConsoleDisplayRuntime& runtime) {
    if (visible != 0u) {
        for (std::uint32_t index = 0u; index < 6u; ++index) {
            auto enabled = runtime.ActorProperty(actor_context, index);
            if (index == 3u)
                enabled = static_cast<std::uint8_t>(
                    enabled & static_cast<std::uint8_t>(
                        runtime.HasAvailableMagic()));
            runtime.SetEnabled(
                runtime.ReadWord(window + 0x150u + 4u * index), enabled);
        }
        for (std::uint32_t index = 0u; index < 6u; ++index) {
            const auto button = runtime.ReadWord(
                window + 0x150u + 4u * index);
            runtime.WriteByte(button + 0x4b1u, 0u);
            runtime.RequestRedraw(button);
        }
        for (std::uint32_t index = 0u; index < 6u; ++index) {
            const auto button = runtime.ReadWord(
                window + 0x150u + 4u * index);
            if (runtime.IsDisabled(button, false)) continue;
            runtime.WriteWord(window + 0x3cu, index);
            runtime.WriteWord(window + 0x1e4u,
                              window + 0x168u + 20u * index);
            runtime.WriteByte(button + 0x4b1u, 1u);
            runtime.RequestRedraw(button);
            break;
        }
    } else {
        for (std::uint32_t index = 0u; index < 6u; ++index)
            runtime.SetEnabled(
                runtime.ReadWord(window + 0x150u + 4u * index), 0u);
    }
    runtime.SetVisible(runtime.ReadWord(window + 0x1e0u), visible);
    return runtime.SetInputActivity(window+0x28u,visible!=0u);
}

}  // namespace pal4
