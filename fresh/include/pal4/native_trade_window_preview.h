#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
namespace pal4 {
class NativeTradeWindowLayout;class NativeUiCollectionObjects;class NativePkFileManagerRuntime;
class NativeTradeWindowPreview final {
public:
    NativeTradeWindowPreview(NativeTradeWindowLayout&,NativeUiCollectionObjects&,NativePkFileManagerRuntime&,std::uint32_t manager);
    ~NativeTradeWindowPreview();
    NativeTradeWindowPreview(const NativeTradeWindowPreview&)=delete;
    NativeTradeWindowPreview& operator=(const NativeTradeWindowPreview&)=delete;
    void Update(std::optional<std::string_view> model,std::optional<std::string_view> texture={},std::optional<std::string_view> path={});
    void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
