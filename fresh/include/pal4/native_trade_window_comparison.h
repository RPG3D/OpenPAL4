#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeTradeWindowLayout;class NativeInventorySession;class NativeBaseObjectMemory;
class NativePlayerScriptRefreshRuntime;class NativePlayerSkillRuntime;struct PlayerManagerObjectState;
class NativeTradeWindowComparison final {
public:
    NativeTradeWindowComparison(NativeTradeWindowLayout&,NativeInventorySession&,PlayerManagerObjectState&,
        NativeBaseObjectMemory&,NativePlayerScriptRefreshRuntime&,NativePlayerSkillRuntime&);
    ~NativeTradeWindowComparison();
    NativeTradeWindowComparison(const NativeTradeWindowComparison&)=delete;
    NativeTradeWindowComparison& operator=(const NativeTradeWindowComparison&)=delete;
    void Update(std::uint32_t slot,std::uint32_t player,std::uint32_t item);
    void Hide(std::uint32_t slot);
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
