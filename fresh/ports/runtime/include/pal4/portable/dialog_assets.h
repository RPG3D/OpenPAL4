#pragma once
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include <cstdint>

namespace pal4::portable {
struct UiImage {
    std::string member;
    int x{},y{},width{},height{};
    float logical_width{},logical_height{};
};
struct DialogStyle {
    UiImage background;
    UiImage indicator;
    std::string font;
    float font_size{},line_space{};
    std::uint32_t color{};
    float panel_top{},panel_height{};
};
using UiReader=std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>;
// Public TinyXML parses user-owned XML; original ScenarioDialog selects portraits.
class DialogAssets {
public:
    explicit DialogAssets(UiReader reader);
    ~DialogAssets();
    const DialogStyle& Style();
    UiImage Image(const std::string& imageset_file,const std::string& name);
    UiImage Portrait(const std::string& name);
    std::vector<std::uint8_t> Read(std::string_view member);
private:struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace pal4::portable
