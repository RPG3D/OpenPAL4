#include "pal4/rich_text_list_item.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRichTextListItemRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::array<float, 4> assigned_colour{};
    std::array<float, 4> frame_setup_rect{};
    std::array<float, 4> frame_render_rect{};
    std::array<float, 4> image_render_rect{};
    std::array<float, 4> image_colour{};
    std::uint32_t frame_clip = 0u;
    std::uint32_t image_clip = 0u;
    float frame_alpha = 0.0f;
    float image_alpha = 0.0f;
    float ratio = 0.0f;
    std::uint32_t destroyed_frame = 0xffffffffu;

    void ConstructBase(std::uint32_t item,
                       std::uint32_t text,
                       std::uint32_t id,
                       std::uint32_t data,
                       bool disabled,
                       bool auto_delete) override {
        Check(item == 0x1000u && text == 0x5000u &&
                  id == 0u && data == 0u && !disabled && auto_delete,
              "CEGUI ListboxItem constructor arguments");
        calls.emplace_back("base");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        calls.emplace_back("float:" + std::to_string(address));
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read_float:" + std::to_string(address));
        return floats[address];
    }
    std::uint32_t CreateFrame() override {
        calls.emplace_back("create_frame");
        return 0x2000u;
    }
    void DestroyFrame(std::uint32_t frame) override {
        destroyed_frame = frame;
        calls.emplace_back("destroy_frame");
    }
    std::uint32_t FindFont(std::string_view name) override {
        Check(name == "system", "system font lookup");
        calls.emplace_back("font");
        return 0x3000u;
    }
    void SetFormatForSC(std::uint32_t frame,
                        bool enabled) override {
        Check(frame == 0x2000u && enabled,
              "rich-text SC formatting");
        calls.emplace_back("format");
    }
    void AssignFrameText(std::uint32_t frame,
                         std::uint32_t text,
                         std::uint32_t font,
                         const std::array<float, 4>& colour) override {
        Check(frame == 0x2000u && text == 0x5000u && font == 0x3000u,
              "frame text arguments");
        assigned_colour = colour;
        calls.emplace_back("assign_text");
    }
    void SetFrameRect(std::uint32_t frame,
                      const std::array<float, 4>& rect,
                      std::uint32_t font) override {
        Check(frame == 0x2000u && font == 0x3000u,
              "frame rectangle arguments");
        frame_setup_rect = rect;
        calls.emplace_back("frame_rect");
    }
    std::uint32_t FindImage(std::string_view imageset,
                            std::string_view image) override {
        Check(imageset == "gy0" && image == "fenglanxian",
              "separator image resource");
        calls.emplace_back("image");
        return 0x4000u;
    }
    std::uint32_t BaseItemVtable() override { return 0x8412f4u; }
    void DestroyString(std::uint32_t address) override {
        calls.emplace_back("string:" + std::to_string(address));
    }
    void RenderFrame(std::uint32_t frame,
                     std::uint32_t context,
                     const std::array<float, 4>& rect,
                     float alpha, float inset,
                     std::uint32_t clip) override {
        Check(frame == 0x2000u && context == 0x7000u &&
                  inset == 10.0f,
              "frame render arguments");
        frame_render_rect = rect;
        frame_alpha = alpha;
        frame_clip = clip;
        calls.emplace_back("render_frame");
    }
    void CacheImage(std::uint32_t cache,
                    std::uint32_t image,
                    const std::array<float, 4>& rect,
                    float alpha, std::uint32_t clip,
                    const std::array<float, 4>& colour,
                    bool first_flag, bool second_flag) override {
        Check(cache == 0x7000u && image == 0x4000u &&
                  !first_flag && !second_flag,
              "CEGUI image cache arguments");
        image_render_rect = rect;
        image_alpha = alpha;
        image_clip = clip;
        image_colour = colour;
        calls.emplace_back("cache_image");
    }
    float GetTextHeight(std::uint32_t frame,
                        float inset) override {
        Check(frame == 0x2000u && inset == 10.0f,
              "frame text-height argument");
        calls.emplace_back("text_height");
        return 13.0f;
    }
    std::array<float, 4> GetParentRect(
        std::uint32_t parent, std::uint32_t virtual_slot) override {
        Check(parent == 0x6000u && virtual_slot == 276u,
              "parent virtual rectangle slot");
        calls.emplace_back("parent_rect");
        return {10.0f, 20.0f, 100.0f, 120.0f};
    }
    std::int32_t FloatToLong(float value) override {
        ratio = value;
        calls.emplace_back("ftol");
        return static_cast<std::int32_t>(value);
    }
};

std::size_t Find(const std::vector<std::string>& calls,
                 const std::string& target) {
    for (std::size_t i = 0; i < calls.size(); ++i)
        if (calls[i] == target) return i;
    throw std::runtime_error("missing call: " + target);
}

void CheckConstructAndDestruct() {
    Runtime runtime;
    runtime.words[0x1000u + 428u] = 0xabcdu;
    Check(pal4::RichTextListItem::Construct(
              0x1000u, 0x5000u, 120.0f, runtime) == 0x1000u &&
              runtime.words[0x1000u] ==
                  pal4::RichTextListItem::kVtable &&
              runtime.words[0x1000u + 424u] == 0x2000u &&
              runtime.words[0x1000u + 432u] == 0x4000u &&
              runtime.words[0x1000u + 428u] == 0xabcdu &&
              runtime.assigned_colour ==
                  (std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}) &&
              runtime.frame_setup_rect ==
                  (std::array<float, 4>{0.0f, 0.0f, 100.0f, 0.0f}) &&
              Find(runtime.calls, "base") <
                  Find(runtime.calls, "create_frame") &&
              Find(runtime.calls, "font") <
                  Find(runtime.calls, "format") &&
              Find(runtime.calls, "format") <
                  Find(runtime.calls, "assign_text") &&
              Find(runtime.calls, "frame_rect") <
                  Find(runtime.calls, "image"),
          "constructor fields, resources and call order");
    runtime.calls.clear();
    pal4::RichTextListItem::Destruct(0x1000u, runtime);
    Check(runtime.destroyed_frame == 0x2000u &&
              runtime.words[0x1000u + 424u] == 0u &&
              runtime.words[0x1000u] == 0x8412f4u &&
              Find(runtime.calls, "destroy_frame") <
                  Find(runtime.calls, "string:4252") &&
              Find(runtime.calls, "string:4252") <
                  Find(runtime.calls, "string:4100"),
          "frame and CEGUI string destruction order");
}

void CheckRender() {
    Runtime runtime;
    runtime.words[0x1000u + 424u] = 0x2000u;
    runtime.words[0x1000u + 432u] = 0x4000u;
    runtime.floats[0x4000u + 32u] = 6.0f;
    const std::array<float, 4> rect{0.0f, 10.0f, 100.0f, 60.0f};
    pal4::RichTextListItem::Render(
        0x1000u, 0x7000u, rect, 0.75f, 0.5f, 0x9000u,
        runtime);
    Check(runtime.frame_render_rect ==
              (std::array<float, 4>{0.0f, 10.0f, 110.0f, 50.0f}) &&
              runtime.image_render_rect ==
              (std::array<float, 4>{4.0f, 10.0f, 100.0f, 60.0f}) &&
              runtime.image_colour ==
              (std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f}) &&
              runtime.frame_alpha == 0.75f &&
              runtime.image_alpha == 0.75f &&
              runtime.frame_clip == 0x9000u &&
              runtime.image_clip == 0x9000u &&
              Find(runtime.calls, "render_frame") <
                  Find(runtime.calls, "cache_image"),
          "frame and separator render geometry");
}

void CheckGetSize(bool image_present) {
    Runtime runtime;
    runtime.words[0x1000u + 424u] = 0x2000u;
    runtime.words[0x1000u + 316u] = 0x6000u;
    runtime.words[0x1000u + 432u] =
        image_present ? 0x4000u : 0u;
    runtime.floats[0x4000u + 32u] = 6.0f;
    runtime.floats[0x1000u + 428u] = 80.0f;
    Check(pal4::RichTextListItem::GetSize(
              0x1000u, 0x7000u, runtime) == 0x7000u &&
              runtime.floats[0x7000u] == 80.0f &&
              runtime.floats[0x7004u] ==
                  30.0f &&
              std::abs(runtime.ratio -
                       (image_present ? 2.9f : 2.3f)) < 0.0001f &&
              Find(runtime.calls, "text_height") <
                  Find(runtime.calls, "parent_rect") &&
              Find(runtime.calls, "parent_rect") <
                  Find(runtime.calls, "ftol"),
          "size ratio, FPU conversion and output pair");
}

}  // namespace

int main() {
    try {
        CheckConstructAndDestruct();
        CheckRender();
        CheckGetSize(true);
        CheckGetSize(false);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
