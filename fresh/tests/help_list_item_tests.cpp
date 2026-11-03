#include "pal4/help_list_item.h"

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

struct Runtime final : pal4::IHelpListItemRuntime {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    pal4::HelpItemRect expected_base_rect{10.0f, 110.0f, 20.0f, 220.0f};
    pal4::HelpItemRect cached_rect{};
    float cached_alpha{};
    bool fail_image{};
    bool fail_base{};

    void ConstructBase(std::uint32_t item, std::uint32_t list,
                       std::u32string_view text) override {
        calls.emplace_back("construct-base");
        Check(item == 0xc000u && list == 0x5003u && text == U"help",
              "base constructor arguments");
        if (fail_base) throw std::runtime_error("base");
    }
    void DestroyBase(std::uint32_t item) override {
        calls.emplace_back("destroy-base");
        Check(item == 0xc000u, "base cleanup address");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t ResolveImage(std::string_view set,
                               std::string_view name) override {
        calls.emplace_back("resolve-image");
        Check(set == "gy0" && name == "equiped", "unread image identity");
        if (fail_image) throw std::runtime_error("image");
        return 0xe000u;
    }
    std::uint32_t TextItemVtable() override {
        calls.emplace_back("text-vtable");
        return 0xa100u;
    }
    std::uint32_t ItemVtable() override {
        calls.emplace_back("item-vtable");
        return 0xa200u;
    }
    void DestroyString(std::uint32_t address) override {
        calls.emplace_back(address == 0xc09cu
            ? "destroy-text" : "destroy-name");
        Check(address == 0xc09cu || address == 0xc004u,
              "base string offsets");
    }
    void DrawBase(std::uint32_t item, std::uint32_t cache,
                  const pal4::HelpItemRect& rect, float alpha,
                  std::uint32_t mode, std::uint32_t clip,
                  std::uint32_t flags) override {
        calls.emplace_back("draw-base");
        Check(item == 0xc000u && cache == 0xf000u &&
                  rect.left == expected_base_rect.left &&
                  rect.right == expected_base_rect.right &&
                  rect.top == expected_base_rect.top &&
                  rect.bottom == expected_base_rect.bottom &&
                  alpha == 0.5f && mode == 1u && clip == 2u && flags == 3u,
              "base draw forwarding");
    }
    pal4::HelpImageSize ImageSize(std::uint32_t image) override {
        calls.emplace_back("image-size");
        Check(image == 0xe000u, "image size address");
        return {20.0f, 10.0f};
    }
    void CacheImage(std::uint32_t cache, std::uint32_t image,
                    const pal4::HelpItemRect& rect,
                    float alpha) override {
        calls.emplace_back("cache-image");
        Check(cache == 0xf000u && image == 0xe000u,
              "cache image arguments");
        cached_rect = rect;
        cached_alpha = alpha;
    }
};

void CheckConstruction() {
    using Item = pal4::HelpListItem;
    Runtime runtime;
    Check(Item::Construct(0xc000u, 0x5003u, U"help", 1u, runtime) ==
              0xc000u &&
              runtime.words.at(0xc000u) == Item::kVtable &&
              runtime.words.at(0xc210u) == 0xe000u &&
              runtime.calls == (std::vector<std::string>{
                  "construct-base", "write", "resolve-image", "write"}),
          "constructor installs vtable and unread image");
    runtime.calls.clear();
    Item::SetImage(0xc000u, 0u, runtime);
    Check(runtime.words.at(0xc210u) == 0u &&
              runtime.calls == (std::vector<std::string>{"write"}),
          "read item clears image without lookup");

    Runtime image_failure;
    image_failure.fail_image = true;
    bool caught = false;
    try { (void)Item::Construct(0xc000u, 0x5003u, U"help", 1u,
                                image_failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && image_failure.calls.back() == "destroy-base",
          "failed image lookup unwinds completed base");
    Runtime base_failure;
    base_failure.fail_base = true;
    caught = false;
    try { (void)Item::Construct(0xc000u, 0x5003u, U"help", 1u,
                                base_failure); }
    catch (const std::runtime_error&) { caught = true; }
    Check(caught && base_failure.calls ==
              (std::vector<std::string>{"construct-base"}),
          "failed base constructor has no derived cleanup");
}

void CheckDestruction() {
    Runtime runtime;
    pal4::HelpListItem::Destroy(0xc000u, runtime);
    Check(runtime.words.at(0xc000u) == 0xa200u &&
              runtime.calls == (std::vector<std::string>{
                  "text-vtable", "write", "item-vtable", "write",
                  "destroy-text", "destroy-name"}),
          "destructor restores base vtables and destroys strings in order");
}

void CheckDrawing() {
    using Item = pal4::HelpListItem;
    Runtime runtime;
    const pal4::HelpItemRect rect{10.0f, 110.0f, 20.0f, 220.0f};
    runtime.words[0xc210u] = 0u;
    Item::Draw(0xc000u, 0xf000u, rect, 0.5f, 1u, 2u, 3u, runtime);
    Check(runtime.calls ==
              (std::vector<std::string>{"draw-base", "read"}),
          "base draws even without overlay image");
    runtime.calls.clear();
    runtime.words[0xc210u] = 0xe000u;
    Item::Draw(0xc000u, 0xf000u, rect, 0.5f, 1u, 2u, 3u, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "draw-base", "read", "image-size", "cache-image"}) &&
              runtime.cached_rect.left == 150.0f &&
              runtime.cached_rect.right == 170.0f &&
              runtime.cached_rect.top == 113.0f &&
              runtime.cached_rect.bottom == 123.0f &&
              runtime.cached_alpha == 0.5f,
          "original Rect ABI: left+140, vertically centered with two-pixel adjustment");
    runtime.calls.clear();
    const pal4::HelpItemRect narrow{10.0f, 110.0f, 20.0f, 25.0f};
    runtime.expected_base_rect = narrow;
    Item::Draw(0xc000u, 0xf000u, narrow, 0.5f, 1u, 2u, 3u, runtime);
    Check(runtime.cached_rect.left == 150.0f &&
              runtime.cached_rect.top == 20.0f && runtime.cached_rect.bottom==30.0f,
          "short row retains its top while the icon keeps its horizontal offset");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckDestruction();
        CheckDrawing();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
