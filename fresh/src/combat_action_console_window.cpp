#include "pal4/combat_action_console_window.h"
#include "pal4/combat_action_console_window_events.h"

#include <array>

namespace pal4 {
namespace {

struct PointerLink {
    std::uint32_t field;
    std::uint32_t target;
};

constexpr std::array<PointerLink, 19> kLinks{{
    {0x1b4u, 0x168u}, {0x1c0u, 0x168u}, {0x194u, 0x168u},
    {0x1a8u, 0x1b8u}, {0x1c8u, 0x17cu}, {0x16cu, 0x1b8u},
    {0x178u, 0x17cu}, {0x180u, 0x1b8u}, {0x188u, 0x1b8u},
    {0x1d0u, 0x17cu}, {0x1acu, 0x190u}, {0x1c4u, 0x1a4u},
    {0x170u, 0x190u}, {0x174u, 0x1a4u}, {0x1a0u, 0x1ccu},
    {0x19cu, 0x1a4u}, {0x184u, 0x1ccu}, {0x1d4u, 0x190u},
    {0x1d8u, 0x190u},
}};

void Unwind(const std::uint32_t window, const std::uint32_t stage,
            ICombatActionConsoleWindowRuntime& runtime) {
    if (stage >= 5u) runtime.DestroyStuntSelect(window + 0x110u);
    if (stage >= 4u) runtime.DestroyPropertySelect(window + 0xb4u);
    if (stage >= 3u) runtime.DestroyMagicSelect(window + 0x5cu);
    if (stage >= 2u) runtime.CleanupNavigation(window + 0x28u);
    if (stage >= 1u) (void)runtime.DestructLayout(window);
}

}  // namespace

std::uint32_t CombatActionConsoleWindow::Construct(
    const std::uint32_t window, const std::uint32_t name,
    ICombatActionConsoleWindowRuntime& runtime) {
    std::uint32_t stage = 0u;
    try {
        runtime.LoadLayout(window, name, 0u, 0u);
        stage = 1u;
        runtime.ConstructNavigation(window + 0x28u);
        stage = 2u;
        for (std::uint32_t offset = 0x40u; offset <= 0x54u;
             offset += 4u)
            runtime.WriteWord(window + offset, 0u);
        runtime.WriteWord(window + 0x58u, 0u);
        runtime.ConstructMagicSelect(window + 0x5cu);
        stage = 3u;
        runtime.ConstructPropertySelect(window + 0xb4u);
        stage = 4u;
        runtime.ConstructStuntSelect(window + 0x110u);
        stage = 5u;

        for (std::uint32_t index = 0u; index < 6u; ++index)
            for (std::uint32_t field = 0u; field < 5u; ++field)
                runtime.WriteWord(window + 0x168u + 20u * index +
                                      4u * field, 0u);
        runtime.WriteWord(window, kVtable);
        runtime.WriteWord(window + 0x28u, kNavigationVtable);
        runtime.SetupUIElements(window);
        (void)runtime.UpdateVisibility(window, 0u, 0u);

        for (std::uint32_t index = 0u; index < 6u; ++index) {
            const auto button = runtime.ReadWord(window + 0x150u + 4u * index);
            runtime.SetupMouseHandlers(window + 0x28u, button);
            runtime.WriteWord(window + 0x168u + 20u * index, button);
        }
        for (std::size_t index = 0u; index < kLinks.size(); ++index) {
            if (index == 10u) runtime.WriteWord(window + 0x1e4u, 0u);
            const auto& link = kLinks[index];
            runtime.WriteWord(window + link.field, window + link.target);
        }
        return window;
    } catch (...) {
        Unwind(window, stage, runtime);
        throw;
    }
}

std::uint32_t CombatActionConsoleWindow::Destruct(
    const std::uint32_t window, ICombatActionConsoleWindowRuntime& runtime) {
    runtime.DestructSelectorLayout(window + 0x110u);
    runtime.DestructSelectorLayout(window + 0xb4u);
    runtime.DestructSelectorLayout(window + 0x5cu);
    const auto navigation = window + 0x28u;
    runtime.FreeMemory(runtime.ReadWord(navigation + 8u));
    runtime.WriteWord(navigation + 8u, 0u);
    runtime.WriteWord(navigation + 12u, 0u);
    runtime.WriteWord(navigation + 16u, 0u);
    return runtime.DestructLayout(window);
}

std::uint32_t CombatActionConsoleWindow::SetParentAndEvents(
    const std::uint32_t window, const std::uint8_t active,
    const std::uint32_t parent, ICombatActionConsoleWindowRuntime& runtime,
    IEventSubscriptionRuntime& event_runtime) {
    class Adapter final:public ICombatActionFrameServices {
        ICombatActionConsoleWindowRuntime& r;IEventSubscriptionRuntime& events;
    public:
        Adapter(ICombatActionConsoleWindowRuntime& r,IEventSubscriptionRuntime& e):r(r),events(e){}
        void SetParent(std::uint32_t w,std::uint8_t a,std::uint32_t p)override{r.SetParent(w,a,p);}
        void RegisterEvents(std::uint32_t w)override{(void)CombatActionConsoleWindowEvents::Initialize(w,events);}
        void CleanupEvents(std::uint32_t w)override{(void)CombatActionConsoleWindowEvents::Cleanup(w,events);}
        std::uint32_t UpdateVisibility(std::uint32_t w,std::uint32_t c,std::uint32_t v)override{return r.UpdateVisibility(w,c,v);}
    } adapter(runtime,event_runtime);
    return SetParentAndEvents(window,active,parent,adapter);
}

std::uint32_t CombatActionConsoleWindow::SetParentAndEvents(std::uint32_t window,
    std::uint8_t active,std::uint32_t parent,ICombatActionFrameServices& runtime){
    runtime.SetParent(window,active,parent);
    if(active)runtime.RegisterEvents(window);else runtime.CleanupEvents(window);
    return runtime.UpdateVisibility(window,0u,0u);
}

} // namespace pal4
