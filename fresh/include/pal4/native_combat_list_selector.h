#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>
namespace pal4 {
class NativeCombatSelectorLayout;class NativeMagicWindowRecords;class NativeGameDatabase;class NativeEventHandlerArray;class NativeInventorySession;
// Borrows actual layout/records/activities. Owns DLL list cells and scoped cursors.
class NativeCombatListSelector final {
public:
    using Learned=std::function<std::vector<std::uint32_t>(std::uint32_t,std::uint32_t)>;
    using Stat=std::function<std::int32_t(std::uint32_t,std::uint32_t)>;
    using Selected=std::function<void(std::uint32_t)>;
    struct StuntServices {
        std::function<std::vector<std::uint32_t>(std::uint32_t)> learned;
        std::function<bool(std::uint32_t,std::uint32_t)> usable;
    };
    NativeCombatListSelector(NativeCombatSelectorLayout&,const NativeMagicWindowRecords&,
        const NativeGameDatabase&,NativeEventHandlerArray&,std::uint32_t manager,
        std::uint32_t action,Learned,Stat,Selected);
    NativeCombatListSelector(NativeCombatSelectorLayout&,NativeInventorySession&,
        const NativeGameDatabase&,NativeEventHandlerArray&,std::uint32_t manager,std::uint32_t action,Selected);
    NativeCombatListSelector(NativeCombatSelectorLayout&,const NativeMagicWindowRecords&,
        const NativeGameDatabase&,NativeEventHandlerArray&,std::uint32_t manager,
        std::uint32_t action,StuntServices,Selected);
    ~NativeCombatListSelector();
    void Setup(std::uint32_t actor);
    void Reset();
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t InvokeVirtual(std::uint32_t,std::uint32_t);
    std::uint8_t InvokePredicate(std::uint32_t,std::uint32_t,std::uint32_t);
    std::size_t ItemCount()const noexcept;
    std::size_t ActiveIterators()const noexcept;
    void Close();
private:
    class Impl;std::shared_ptr<Impl> impl_;
};
}
