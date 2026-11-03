#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeTradeWindowLayout;class NativeTradeWindowData;
class NativeTradeWindowPropertyPages;class NativeTradeWindowRecordPages;
class NativeTradeWindowNavigation final {
public:
    NativeTradeWindowNavigation(NativeTradeWindowLayout&,NativeTradeWindowData&,
        NativeTradeWindowPropertyPages&,NativeTradeWindowRecordPages&,std::function<void()> before_change);
    ~NativeTradeWindowNavigation();
    NativeTradeWindowNavigation(const NativeTradeWindowNavigation&)=delete;
    NativeTradeWindowNavigation& operator=(const NativeTradeWindowNavigation&)=delete;
    void Update(std::uint32_t group,std::uint32_t first,bool forward);
    bool Event(std::uint32_t method);
    // Retire before data, page and layout owners.
    void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
