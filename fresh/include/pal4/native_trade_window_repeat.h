#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeTradeWindowLayout;
class NativeNumericSpinnerPool;
class NativeGameTimerQueue;
// Borrows the frame, real quantity rows and existing UIFrameManager timer queue.
// Route frame CC20/CC80/CCE0 and queue CD20/CD80 callbacks to this owner.
class NativeTradeWindowRepeat final {
public:
    NativeTradeWindowRepeat(NativeTradeWindowLayout&,NativeNumericSpinnerPool&,NativeGameTimerQueue&,std::uint32_t manager);
    ~NativeTradeWindowRepeat();
    NativeTradeWindowRepeat(const NativeTradeWindowRepeat&)=delete;
    NativeTradeWindowRepeat& operator=(const NativeTradeWindowRepeat&)=delete;
    bool Event(std::uint32_t method,std::uint32_t event);
    void Callback(std::uint32_t owner,std::uint32_t method,std::int32_t row,std::uint8_t decrement);
    // Cancel before a page replaces its borrowed rows or closes its layout.
    void Cancel();
    void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
