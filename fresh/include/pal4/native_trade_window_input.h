#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeTradeWindowLayout;
struct NativeTradeWindowInputServices {
    std::function<void()> close;
    std::function<void()> prompt;
    // Borrow the row owner's native ListboxSpinnerItem operations. This input
    // adapter never creates a second quantity store or owns row items.
    std::function<std::int32_t(std::uint32_t)> spinner_value;
    std::function<void(std::uint32_t,std::int32_t)> set_spinner_value;
};
class NativeTradeWindowInput final {
public:
    NativeTradeWindowInput(NativeTradeWindowLayout&,NativeTradeWindowInputServices);
    ~NativeTradeWindowInput();
    NativeTradeWindowInput(const NativeTradeWindowInput&)=delete;
    NativeTradeWindowInput& operator=(const NativeTradeWindowInput&)=delete;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t member,std::uint32_t slot);
    std::uint8_t Predicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
