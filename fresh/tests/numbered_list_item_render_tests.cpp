#include "pal4/numbered_list_item.h"
#include "pal4/numbered_list_item_render.h"
#include "pal4/time_formatter.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using Rect = std::array<float, 4>;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ImageCall {
    std::uint32_t image;
    Rect rect;
    Rect clip;
    float alpha;
    bool first;
    bool second;
};

struct TextCall {
    std::string text;
    pal4::NumberedTextEncoding encoding;
    std::uint32_t format;
    Rect rect;
    Rect clip;
    float alpha;
};

struct Runtime final : pal4::INumberedListRenderRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<pal4::NumberedGameRecord> records;
    std::vector<ImageCall> images;
    std::vector<TextCall> texts;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> icon_slots;
    std::uint32_t font = 0x3000u;
    std::uint32_t item_text_address = 0u;
    Rect item_text_rect{};
    Rect item_text_clip{};
    float item_text_alpha = 0.0f;
    bool trigger_result = false;
    bool trigger_player_called = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    float ReadFloat(std::uint32_t address) override {
        return floats[address];
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x7000u, "name data pointer");
        calls.emplace_back("name");
        return "Hero";
    }
    std::uint32_t GetSystemFont() override {
        calls.emplace_back("font");
        return font;
    }
    std::uint32_t ModulatedBlack(std::uint32_t item,
                                 float modulation) override {
        Check(item == 0x1000u && modulation == 0.5f,
              "item colour modulation");
        calls.emplace_back("modulate");
        return 0xa000u;
    }
    void CacheWhiteImage(std::uint32_t cache,
                         std::uint32_t image,
                         const Rect& rect,
                         float alpha, const Rect& clip,
                         bool first_flag,
                         bool second_flag) override {
        Check(cache == 0x2000u, "render cache handle");
        images.push_back({image, rect, clip, alpha,
                          first_flag, second_flag});
        calls.emplace_back("image:" + std::to_string(image));
    }
    void CacheItemText(std::uint32_t cache,
                       std::uint32_t text_address,
                       std::uint32_t text_font,
                       std::uint32_t format,
                       const Rect& rect, float alpha,
                       const Rect& clip,
                       std::uint32_t colour) override {
        Check(cache == 0x2000u && text_font == 0x3000u &&
                  format == 2u && colour == 0xa000u,
              "number text cache arguments");
        item_text_address = text_address;
        item_text_rect = rect;
        item_text_clip = clip;
        item_text_alpha = alpha;
        calls.emplace_back("item_text");
    }
    void CacheBlackText(std::uint32_t cache,
                        std::string_view text,
                        pal4::NumberedTextEncoding encoding,
                        std::uint32_t text_font,
                        std::uint32_t format,
                        const Rect& rect, float alpha,
                        const Rect& clip) override {
        Check(cache == 0x2000u && text_font == 0x3000u,
              "detail text cache arguments");
        texts.push_back({std::string(text), encoding, format,
                         rect, clip, alpha});
        calls.emplace_back("text:" + std::string(text));
    }
    std::uint32_t GetIconImage(std::uint32_t slot,
                               std::uint32_t category,
                               std::uint32_t variant) override {
        Check(category == 6u && variant == 2u,
              "icon category and variant");
        icon_slots.push_back(slot);
        calls.emplace_back("icon:" + std::to_string(slot));
        return 0x5000u + slot;
    }
    Rect IntersectRect(const Rect& rect,
                       const Rect& clip) override {
        Check(rect[0] == 13.0f && clip[1] == 24.0f,
              "icon intersection inputs");
        calls.emplace_back("intersect");
        return clip;
    }
    std::string FormatItemCount(std::uint32_t trigger,
                                std::uint32_t count) override {
        Check(trigger == 0xc9u, "item count trigger");
        calls.emplace_back("format_count");
        return "Count:" + std::to_string(count);
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        return std::string(text);
    }
    std::vector<pal4::NumberedGameRecord> GetGameRecords() override {
        calls.emplace_back("records");
        return records;
    }
    bool TriggerObject(std::uint32_t id) override {
        Check(id == 17u, "trigger object id");
        calls.emplace_back("trigger_object");
        return trigger_result;
    }
    void TriggerPlayer(std::uint32_t id) override {
        Check(id == 17u, "trigger player id");
        trigger_player_called = true;
        calls.emplace_back("trigger_player");
    }
};

Runtime BaseRuntime(bool detailed, bool selected) {
    Runtime runtime;
    runtime.words[0x1000u + 504u] = 0x4000u;
    runtime.words[0x1000u + 496u] = 0x4100u;
    runtime.words[0x1000u + 500u] = 0x4200u;
    runtime.words[0x1000u + 492u] = detailed ? 1u : 0u;
    runtime.bytes[0x1000u + 423u] = selected ? 1u : 0u;
    runtime.floats[0x4000u + 28u] = 80.0f;
    runtime.floats[0x4000u + 32u] = 20.0f;
    runtime.words[0x1000u + 488u] = 17u;
    return runtime;
}

constexpr Rect kRect{10.0f, 20.0f, 100.0f, 150.0f};
constexpr Rect kClip{0.0f, 20.0f, 200.0f, 300.0f};
constexpr Rect kAdjustedClip{0.0f, 24.0f, 200.0f, 300.0f};
constexpr Rect kFrameRect{10.0f, 30.0f, 100.0f, 180.0f};
constexpr Rect kNumberRect{47.0f, 60.0f, 107.0f, 121.0f};

void CheckBasic() {
    auto runtime = BaseRuntime(false, true);
    pal4::NumberedListItem::Render(
        0x1000u, 0x2000u, kRect, 0.75f, 0.5f,
        kClip, runtime);
    Check(runtime.images.size() == 2u &&
              runtime.images[0].image == 0x4100u &&
              runtime.images[1].image == 0x4200u &&
              runtime.images[0].rect == kFrameRect &&
              runtime.images[1].rect == kFrameRect &&
              runtime.images[0].clip == kAdjustedClip &&
              runtime.images[0].first &&
              !runtime.images[0].second &&
              runtime.item_text_address == 0x1000u + 4u &&
              runtime.item_text_rect == kNumberRect &&
              runtime.item_text_clip == kAdjustedClip &&
              runtime.item_text_alpha == 0.75f &&
              runtime.texts.empty() &&
              runtime.icon_slots.empty(),
          "basic border, number and selection overlay");

    auto no_font = BaseRuntime(false, true);
    no_font.font = 0u;
    pal4::NumberedListItem::Render(
        0x1000u, 0x2000u, kRect, 0.75f, 0.5f,
        kClip, no_font);
    Check(no_font.images.empty() && no_font.texts.empty() &&
              no_font.item_text_address == 0u,
          "missing system font skips cache work");
}

void CheckDetailed() {
    auto runtime = BaseRuntime(true, true);
    runtime.words[0x1000u + 508u] = 0x4300u;
    runtime.words[0x1000u + 444u] = 1u;
    runtime.words[0x1000u + 448u] = 1u;
    runtime.words[0x1000u + 456u] = 1u;
    runtime.words[0x1000u + 464u] = 2u;
    runtime.words[0x1000u + 472u] = 5u;
    runtime.words[0x1000u + 480u] = 5u;
    runtime.words[0x1000u + 484u] = 65u;
    runtime.words[0x1000u + 428u] = 0x7000u;
    runtime.trigger_result = true;
    runtime.records = {
        {-1, "old", "old2"},
        {5, "middle", "middle2"},
        {9, "late", "late2"},
        {4, "last", "last2"}};
    pal4::NumberedListItem::Render(
        0x1000u, 0x2000u, kRect, 0.75f, 0.5f,
        kClip, runtime);

    Check(runtime.images.size() == 5u &&
              runtime.images[0].image == 0x4300u &&
              runtime.images[0].rect ==
                  (Rect{17.0f, 92.0f, 595.0f, 693.0f}) &&
              !runtime.images[0].first &&
              runtime.images[1].image == 0x4000u &&
              runtime.images[1].first &&
              runtime.images[2].image == 0x5000u &&
              runtime.images[3].image == 0x5002u &&
              runtime.images[4].image == 0x4200u &&
              runtime.images[2].rect ==
                  (Rect{13.0f, 84.0f, 127.0f, 180.0f}) &&
              runtime.images[3].rect ==
                  (Rect{13.0f, 84.0f, 180.0f, 233.0f}) &&
              runtime.images[2].clip ==
                  (Rect{0.0f, 23.0f, 200.0f, 300.0f}) &&
              runtime.icon_slots ==
                  (std::vector<std::uint32_t>{0u, 2u}),
          "detailed background and sparse icons");
    Check(runtime.texts.size() == 6u &&
              runtime.texts[0].text == "Count:2" &&
              runtime.texts[1].text == "Count:5" &&
              runtime.texts[0].encoding ==
                  pal4::NumberedTextEncoding::Utf8 &&
              runtime.texts[0].rect ==
                  (Rect{80.0f, 91.0f, 127.0f, 180.0f}) &&
              runtime.texts[1].rect ==
                  (Rect{80.0f, 91.0f, 180.0f, 233.0f}) &&
              runtime.texts[2].text == "last" &&
              runtime.texts[2].encoding ==
                  pal4::NumberedTextEncoding::Utf8 &&
              runtime.texts[3].text == "last2" &&
              runtime.texts[3].encoding ==
                  pal4::NumberedTextEncoding::Utf8 &&
              runtime.texts[4].text ==
                  pal4::TimeFormatter::FormatSeconds(65) &&
              runtime.texts[5].text == "Hero" &&
              runtime.trigger_player_called,
          "count, last matching record, time and trigger text");
}

void CheckDetailedGates() {
    auto runtime = BaseRuntime(true, false);
    runtime.words[0x1000u + 480u] =
        static_cast<std::uint32_t>(-1);
    runtime.words[0x1000u + 484u] = 0u;
    runtime.records = {{0, "future", "future2"}};
    pal4::NumberedListItem::Render(
        0x1000u, 0x2000u, kRect, 0.75f, 0.5f,
        kClip, runtime);
    Check(runtime.images.size() == 1u &&
              runtime.images[0].image == 0x4000u &&
              runtime.texts.size() == 1u &&
              runtime.texts[0].text ==
                  pal4::TimeFormatter::FormatSeconds(0) &&
              !runtime.trigger_player_called,
          "null secondary, no counts, no matching record or trigger");
}

void CheckSizePair() {
    auto runtime = BaseRuntime(false, false);
    Check(pal4::NumberedListItem::SetSize(
              0x1000u, 0x12345678u, runtime) == 0x4000u &&
              runtime.words[0x1000u + 512u] == 0x42a00000u &&
              runtime.words[0x1000u + 516u] == 0x12345678u,
          "size setter stores image width bits and second word");
    Check(pal4::NumberedListItem::GetSize(
              0x1000u, 0x8000u, runtime) == 0x8000u &&
              runtime.words[0x8000u] == 0x42a00000u &&
              runtime.words[0x8004u] == 0x12345678u,
          "size getter copies pair and returns destination");
}

}  // namespace

int main() {
    try {
        CheckBasic();
        CheckDetailed();
        CheckDetailedGates();
        CheckSizePair();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
