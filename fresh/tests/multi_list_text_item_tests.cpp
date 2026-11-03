#include "pal4/multi_list_text_item.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

bool ColourEquals(pal4::MultiListColour actual,
                  pal4::MultiListColour expected) {
    return actual.red == expected.red && actual.green == expected.green &&
           actual.blue == expected.blue && actual.alpha == expected.alpha;
}

struct Runtime final : pal4::IMultiListTextItemRuntime {
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::uint32_t font = 0x9000u;
    std::uint32_t colour_rect = 0u;
    std::uint32_t image_cached = 0u;
    std::uint32_t text_cached = 0u;

    void ConstructBase(std::uint32_t item, std::uint32_t text,
                       std::uint32_t id, std::uint32_t data,
                       std::uint32_t disabled,
                       std::uint32_t auto_delete) override {
        calls.emplace_back("base_construct");
        Check(item == 0x1000u && text == 0x2000u && id == 0u &&
              data == 0u && disabled == 0u && auto_delete == 1u,
              "base listbox constructor arguments");
    }
    void InitColourRect(std::uint32_t address,
                        pal4::MultiListColour colour) override {
        calls.emplace_back("colour_rect");
        Check(address == 0x1000u + 524u &&
              ColourEquals(colour, {0.5f, 0.5f, 0.5f, 1.0f}),
              "selection colour rect");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write_word");
        words[address] = value;
    }
    void SetTextColours(std::uint32_t item,
                        pal4::MultiListColour colour) override {
        calls.emplace_back("text_colours");
        Check(item == 0x1000u &&
              ColourEquals(colour, {0.0f, 1.0f, 0.0f, 1.0f}),
              "four green text corners");
    }
    void SetSelectionBrush(std::uint32_t item,
                           std::string_view imageset,
                           std::string_view image) override {
        calls.emplace_back("brush");
        Check(item == 0x1000u && imageset == "OiramLook" &&
              image == "MultiListSelectionBrush",
              "selection brush resource names");
    }
    std::uint32_t BaseTextVtable() override {
        calls.emplace_back("text_vtable");
        return 0xaaaa0000u;
    }
    std::uint32_t BaseItemVtable() override {
        calls.emplace_back("item_vtable");
        return 0xbbbb0000u;
    }
    void DestroyString(std::uint32_t address) override {
        calls.push_back("destroy:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words[address];
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read_float");
        return floats.at(address);
    }
    std::uint32_t ModulateColours(std::uint32_t item,
                                  std::uint32_t base,
                                  std::uint32_t cache,
                                  std::uint32_t mode) override {
        calls.emplace_back("modulate");
        Check(item == 0x1000u && cache == 0x4000u && mode == 3u,
              "colour modulation arguments");
        colour_rect = base;
        return base + 0x10000u;
    }
    void CacheImage(std::uint32_t cache, std::uint32_t image,
                    std::uint32_t area, float inset,
                    float alpha, std::uint32_t colours) override {
        calls.emplace_back("cache_image");
        Check(cache == 0x4000u && image == 0x8000u &&
              area == 0x3000u && inset == 7.0f && alpha == 0.75f &&
              colours == 0x1000u + 320u + 0x10000u,
              "image cache rectangle and alpha arguments");
        image_cached = image;
    }
    std::uint32_t GetFont(std::uint32_t item) override {
        calls.emplace_back("font");
        Check(item == 0x1000u, "font item");
        return font;
    }
    std::uint32_t CacheText(std::uint32_t target, std::uint32_t text,
                            std::uint32_t selected_font,
                            float x, float y, std::uint32_t mode,
                            std::uint32_t colours) override {
        calls.emplace_back("cache_text");
        Check(target == 0x5000u && text == 0x1004u &&
              selected_font == 0x9000u && x == 17.0f && y == 20.0f &&
              mode == 3u && colours == colour_rect + 0x10000u,
              "text cache arguments and seven-pixel inset");
        text_cached = text;
        return 0xabcdef01u;
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::MultiListTextItem::Construct(0x1000u, 0x2000u,
                                               runtime) == 0x1000u,
          "constructor return");
    Check(runtime.words[0x1000u] == pal4::MultiListTextItem::kVtable &&
          runtime.calls == std::vector<std::string>{
              "base_construct", "colour_rect", "write_word",
              "text_colours", "brush"},
          "listbox construction order");
    runtime.calls.clear();
    pal4::MultiListTextItem::Destruct(0x1000u, runtime);
    Check(runtime.words[0x1000u] == 0xbbbb0000u &&
          runtime.calls == std::vector<std::string>{
              "text_vtable", "write_word", "item_vtable", "write_word",
              "destroy:4252", "destroy:4100"},
          "base vtables and text destruction order");
}

void CheckRender(bool image_enabled, bool image_present,
                 bool font_present, bool selected) {
    Runtime runtime;
    runtime.bytes[0x1000u + 420u] = image_enabled;
    runtime.words[0x1000u + 416u] = image_present ? 0x8000u : 0u;
    runtime.bytes[0x1000u + 421u] = selected;
    runtime.floats[0x3000u + 8u] = 10.0f;
    runtime.floats[0x3000u + 12u] = 20.0f;
    runtime.font = font_present ? 0x9000u : 0u;
    const auto result = pal4::MultiListTextItem::Render(
        0x1000u, 0x3000u, 0x5000u, 0x4000u, 3u, 0.75f, runtime);
    Check(result == (font_present ? 0xabcdef01u : 0u),
          "font/no-font return");
    Check((runtime.image_cached == 0x8000u) ==
              (image_enabled && image_present),
          "image cache gate");
    Check((runtime.text_cached == 0x1004u) == font_present,
          "text cache gate");
    if (font_present)
        Check(runtime.colour_rect == 0x1000u +
                  (selected ? 524u : 424u),
              "selected text colour rect");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        for (const bool image_enabled : {false, true})
            for (const bool image_present : {false, true})
                for (const bool font_present : {false, true})
                    for (const bool selected : {false, true})
                        CheckRender(image_enabled, image_present,
                                    font_present, selected);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
