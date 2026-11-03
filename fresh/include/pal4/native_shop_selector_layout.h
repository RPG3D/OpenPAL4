#pragma once
#include "pal4/native_selector_navigation_layout.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
// Original TradeSelectWindow controls and callback ownership. Availability,
// global input and game actions are composed by the selector controller.
class NativeShopSelectorLayout final : public INativeSelectorNavigationLayout {
public:
    using Handler = std::function<bool(std::uint32_t method,std::uint32_t event)>;
    NativeShopSelectorLayout(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,Handler);
    ~NativeShopSelectorLayout();
    NativeShopSelectorLayout(const NativeShopSelectorLayout&)=delete;
    NativeShopSelectorLayout& operator=(const NativeShopSelectorLayout&)=delete;
    std::uint32_t FrameAddress() const noexcept;
    std::uint32_t RootAddress() const noexcept;
    std::uint32_t ImageSize()const noexcept override{return 0xc8u;}
    bool AcceptsInputWindow(std::uint32_t)const noexcept override;
    std::uint32_t& Word(std::uint32_t offset);
    void BindParent(bool attach,std::uint32_t parent=0u);
    void SetupMouseHandlers(std::uint32_t window);
    void SetShopId(std::string_view);
    void SetTaskScript(std::string_view);
    std::string_view ShopId() const noexcept;
    std::string_view TaskScript() const noexcept;
    bool InCallback() const noexcept;
    void RethrowEventError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
