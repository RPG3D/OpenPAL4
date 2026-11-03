#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
namespace pal4 {
class NativeSmithWindowLayout;class NativeUiCollectionObjects;class NativeEffectSceneSession;
class NativePkFileManagerRuntime;class NativeEventHandlerArray;
// Owns original Smith +9C/+A0 model IDs and +A4 effect handle. Borrowed UI
// collection/effects/packages/activity/layout must outlive this owner.
class NativeSmithWindowPreview final {
public:
    NativeSmithWindowPreview(NativeSmithWindowLayout&,NativeUiCollectionObjects&,NativeEffectSceneSession&,
        NativePkFileManagerRuntime&,NativeEventHandlerArray&,std::uint32_t manager);
    ~NativeSmithWindowPreview();
    NativeSmithWindowPreview(const NativeSmithWindowPreview&)=delete;
    NativeSmithWindowPreview& operator=(const NativeSmithWindowPreview&)=delete;
    void Update(std::optional<std::string_view> model,std::optional<std::string_view> texture=std::nullopt,
        std::optional<std::string_view> class_path=std::nullopt);
    void ShowEffectPanel();
    void ReleaseEffect();
    void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
