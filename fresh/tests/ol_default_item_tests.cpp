#include "pal4/ol_default_item.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

struct Runtime final : pal4::IOlDefaultItemRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    pal4::OlRect rendered{};
    std::uint32_t found_image = 0x9000u;
    std::uint32_t cached_image = 0u;

    void ConstructBase(std::uint32_t item, std::uint32_t list,
                       std::uint32_t text) override {
        calls.emplace_back("construct_base");
        Check(item == 0x1000u && list == 0x2000u && text == 0x3000u,
              "base constructor arguments");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_image");
        Check(address == 0x1000u + 528u, "image field offset");
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        calls.push_back("float:" + std::to_string(address));
        return floats.at(address);
    }
    std::uint32_t FindImage(std::string_view imageset,
                            std::string_view image) override {
        calls.emplace_back("find_image");
        Check(imageset == "gy0" && image == "equiped",
              "equipped resource names");
        return found_image;
    }
    std::uint32_t BaseTextVtable() override {
        calls.emplace_back("text_vtable");
        return 0xaaaau;
    }
    std::uint32_t BaseItemVtable() override {
        calls.emplace_back("item_vtable");
        return 0xbbbbu;
    }
    void DestroyString(std::uint32_t address) override {
        calls.push_back("destroy:" + std::to_string(address));
    }
    void DrawBase(std::uint32_t item, std::uint32_t cache,
                  const pal4::OlRect& rect, float alpha, float modulation,
                  std::uint32_t clip, std::uint32_t format) override {
        calls.emplace_back("draw_base");
        Check(item == 0x1000u && cache == 0x4000u &&
              rect == pal4::OlRect{10.0f, 40.0f, 5.0f, 6.0f} &&
              alpha == 0.75f && modulation == 0.5f &&
              clip == 0x5000u && format == 3u,
              "base draw forwarding");
    }
    void CacheImage(std::uint32_t cache, std::uint32_t image,
                    const pal4::OlRect& rect, float alpha,
                    std::uint32_t clip, const std::array<float, 4>& colour,
                    bool first_flag, bool second_flag) override {
        calls.emplace_back("cache_image");
        Check(cache == 0x4000u && alpha == 0.75f &&
              clip == 0x5000u &&
              colour == std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f} &&
              !first_flag && !second_flag, "cache arguments");
        cached_image = image;
        rendered = rect;
    }
};

void CheckLifecycle(bool equipped) {
    Runtime runtime;
    Check(pal4::OlDefaultItem::Construct(0x1000u, 0x2000u, 0x3000u,
                                        equipped, runtime) == 0x1000u,
          "constructor return");
    Check(runtime.words.at(0x1000u) == pal4::OlDefaultItem::kVtable &&
          runtime.words.at(0x1000u + 528u) ==
              (equipped ? 0x9000u : 0u), "constructed fields");
    const std::vector<std::string> expected = equipped
        ? std::vector<std::string>{"construct_base", "write:4096",
                                   "find_image", "write:4624"}
        : std::vector<std::string>{"construct_base", "write:4096",
                                   "write:4624"};
    Check(runtime.calls == expected, "construction order");

    runtime.calls.clear();
    pal4::OlDefaultItem::Destruct(0x1000u, runtime);
    Check(runtime.words.at(0x1000u) == 0xbbbbu &&
          runtime.calls == std::vector<std::string>{
              "text_vtable", "write:4096", "item_vtable", "write:4096",
              "destroy:4252", "destroy:4100"}, "destruction order");
}

void CheckRender(float height, pal4::OlRect expected) {
    Runtime runtime;
    runtime.words[0x1000u + 528u] = 0x9000u;
    runtime.floats[0x9000u + 28u] = 12.0f;
    runtime.floats[0x9000u + 32u] = height;
    pal4::OlDefaultItem::Render(0x1000u, 0x4000u,
                               {10.0f, 40.0f, 5.0f, 6.0f},
                               0.75f, 0.5f, 0x5000u, 3u, runtime);
    Check(runtime.cached_image == 0x9000u && runtime.rendered == expected,
          "image placement");
    Check(runtime.calls.front() == "draw_base" &&
          runtime.calls.back() == "cache_image", "render order");
}

void CheckNoImage() {
    Runtime runtime;
    runtime.words[0x1000u + 528u] = 0u;
    pal4::OlDefaultItem::Render(0x1000u, 0x4000u,
                               {10.0f, 40.0f, 5.0f, 6.0f},
                               0.75f, 0.5f, 0x5000u, 3u, runtime);
    Check(runtime.calls == std::vector<std::string>{"draw_base", "read_image"},
          "empty image path");
}

}  // namespace

int main() {
    try {
        CheckLifecycle(false);
        CheckLifecycle(true);
        CheckNoImage();
        CheckRender(10.0f, {18.0f, 28.0f, 109.0f, 121.0f});
        CheckRender(30.0f, {10.0f, 40.0f, 109.0f, 121.0f});
        CheckRender(40.0f, {10.0f, 50.0f, 109.0f, 121.0f});
        std::cout << "ol_default_item_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
