#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeTradeWindowLayout;class NativeInventorySession;class NativeNumericSpinnerPool;
struct NativeTradePropertyPageServices {
    std::function<void()> before_reset;
    std::function<void()> clear_details;
};
// Property buy/sell pages from493FE0/495080 (legacy CharacterPages name).
// Borrow frame+140 data populated by NativeTradeWindowData and the quantity pool.
class NativeTradeWindowPropertyPages final {
public:
    NativeTradeWindowPropertyPages(NativeTradeWindowLayout&,NativeInventorySession&,
        NativeNumericSpinnerPool&,NativeTradePropertyPageServices);
    ~NativeTradeWindowPropertyPages();
    NativeTradeWindowPropertyPages(const NativeTradeWindowPropertyPages&)=delete;
    NativeTradeWindowPropertyPages& operator=(const NativeTradeWindowPropertyPages&)=delete;
    void Update(std::uint32_t category);
    // Route this pool's change notification here for the original total label.
    void QuantityChanged();
    // Detach cells before another page uses the shared list. Keep columns.
    void Clear();
    void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
