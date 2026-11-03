#pragma once
#include <functional>
#include <memory>
#include <cstdint>
namespace pal4 {
class NativeTradeWindowLayout;class NativeInventorySession;class NativePlayerInventoryStateRuntime;
class NativeGameDatabase;class NativePrescriptionRecordCatalog;
struct NativeTradeRecordPageServices {std::function<void()> before_reset;std::function<void()> clear_details;};
// Equipment and prescription pages share one borrowed DLL list and row owner.
class NativeTradeWindowRecordPages final {
public:
    NativeTradeWindowRecordPages(NativeTradeWindowLayout&,NativeInventorySession&,
        NativePlayerInventoryStateRuntime&,const NativeGameDatabase&,
        const NativePrescriptionRecordCatalog&,NativeTradeRecordPageServices);
    ~NativeTradeWindowRecordPages();
    NativeTradeWindowRecordPages(const NativeTradeWindowRecordPages&)=delete;
    NativeTradeWindowRecordPages& operator=(const NativeTradeWindowRecordPages&)=delete;
    void Equipment(std::uint32_t category);
    void Recipes(std::uint32_t mode);
    void Clear();void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
