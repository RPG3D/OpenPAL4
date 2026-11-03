#pragma once
#include <cstdint>
#include <functional>
#include <memory>
namespace pal4 {
class NativeSmithWindowLayout;class NativeSmithWindowLists;class NativeSmithWindowPreview;class NativeSmithWindowConfirmation;
struct NativeSmithTabServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
};
// Original craft tab/mode/display composition. Model changes are real preview
// updates, not persistence. Close before preview/confirmation/lists/layout.
class NativeSmithWindowTabs final {
public:
    NativeSmithWindowTabs(NativeSmithWindowLayout&,NativeSmithWindowLists&,NativeSmithWindowPreview&,NativeSmithWindowConfirmation&,NativeSmithTabServices);
    ~NativeSmithWindowTabs();
    NativeSmithWindowTabs(const NativeSmithWindowTabs&)=delete;
    NativeSmithWindowTabs& operator=(const NativeSmithWindowTabs&)=delete;
    bool Choose(std::uint32_t mode);
    bool FromDll(std::uint32_t handler);
    bool DispatchCraftTabKey(std::uint32_t event);
    void ResetDisplay();
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
