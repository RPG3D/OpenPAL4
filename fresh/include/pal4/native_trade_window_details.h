#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
namespace pal4 {
class NativeTradeWindowLayout;class NativeInventorySession;class NativeGameDatabase;class NativePrescriptionRecordCatalog;
struct PlayerManagerObjectState;
struct NativeTradeDetailServices {
    std::function<void(std::optional<std::string_view>,std::optional<std::string_view>,std::optional<std::string_view>)> model;
    std::function<void(std::uint32_t slot,std::uint32_t player,std::uint32_t item)> player_slot;
    std::function<void(std::uint32_t slot)> hide_slot;
    std::function<void()> total;
};
class NativeTradeWindowDetails final {
public:
    NativeTradeWindowDetails(NativeTradeWindowLayout&,NativeInventorySession&,PlayerManagerObjectState&,
        const NativeGameDatabase&,const NativePrescriptionRecordCatalog&,NativeTradeDetailServices);
    ~NativeTradeWindowDetails();
    NativeTradeWindowDetails(const NativeTradeWindowDetails&)=delete;
    NativeTradeWindowDetails& operator=(const NativeTradeWindowDetails&)=delete;
    bool Hover();
    void Clear();
    void Equipment(std::uint32_t record);
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
