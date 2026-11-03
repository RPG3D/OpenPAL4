#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/picture_view_window_lifecycle.h"

namespace pal4 {

class IPictureViewWindowDisplayRuntime {
public:
    virtual ~IPictureViewWindowDisplayRuntime() = default;
    virtual void ProcessPictureScript() = 0;
    [[nodiscard]] virtual bool HasImageset(std::string_view name) = 0;
    virtual void DestroyImageset(std::string_view name) = 0;
    [[nodiscard]] virtual std::string RecordImageName(std::uint32_t record) = 0;
    virtual void CreateImageset(std::string_view name, std::string_view file,
                                std::string_view resource_group) = 0;
    virtual void SetImagesetAutoScaling(std::string_view name, bool enabled) = 0;
    virtual void SetImage(std::uint32_t window, std::string_view imageset,
                          std::string_view image) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void CleanupPictureScripts() = 0;
};

class PictureViewWindowDisplayRuntimeStub :
    public IPictureViewWindowDisplayRuntime {
public:
    void ProcessPictureScript() override {}
    [[nodiscard]] bool HasImageset(std::string_view) override { return false; }
    void DestroyImageset(std::string_view) override {}
    [[nodiscard]] std::string RecordImageName(std::uint32_t) override { return {}; }
    void CreateImageset(std::string_view, std::string_view,
                        std::string_view) override {}
    void SetImagesetAutoScaling(std::string_view, bool) override {}
    void SetImage(std::uint32_t, std::string_view,
                  std::string_view) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void SetEnabled(std::uint32_t, bool) override {}
    void CleanupPictureScripts() override {}
};

class PictureViewWindowDisplay final {
public:
    static constexpr std::string_view kImageset = "ui_picture_view_imageset";
    static constexpr std::string_view kImage = "full_image";

    static void Show(PictureViewWindowState& state, std::uint32_t record,
                     IPictureViewWindowDisplayRuntime& runtime);
};

}  // namespace pal4
