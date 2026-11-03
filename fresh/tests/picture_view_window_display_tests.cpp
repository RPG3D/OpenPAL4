#include "pal4/picture_view_window_display.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PictureViewWindowDisplayRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    bool present{};
    bool fail_create{};

    void ProcessPictureScript() override { calls.emplace_back("process"); }
    bool HasImageset(std::string_view name) override {
        Check(name == "ui_picture_view_imageset", "imageset presence name");
        calls.emplace_back("has");
        return present;
    }
    void DestroyImageset(std::string_view name) override {
        Check(name == "ui_picture_view_imageset", "destroy imageset name");
        calls.emplace_back("destroy");
    }
    std::string RecordImageName(std::uint32_t record) override {
        Check(record == 0x9100u, "record address passed to name reader");
        calls.emplace_back("record");
        return "scene.png";
    }
    void CreateImageset(std::string_view name, std::string_view file,
                        std::string_view group) override {
        Check(name == "ui_picture_view_imageset" &&
                  file == "gamedata\\ui\\picture\\scene.png" &&
                  group.empty(), "imageset creation arguments");
        calls.emplace_back("create");
        if (fail_create) throw std::runtime_error("create");
    }
    void SetImagesetAutoScaling(std::string_view name, bool enabled) override {
        Check(name == "ui_picture_view_imageset" && !enabled,
              "imageset auto scaling disabled");
        calls.emplace_back("scale");
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(window == 0x4000u && imageset == "ui_picture_view_imageset" &&
                  image == "full_image", "static image target");
        calls.emplace_back("image");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check(window == 0x3000u || window == 0x2000u,
              "only navigation buttons change enabled state");
        calls.emplace_back(std::string(window == 0x3000u ? "right" : "left") +
                           (enabled ? "+" : "-"));
    }
    void CleanupPictureScripts() override { calls.emplace_back("cleanup"); }
};

pal4::PictureViewWindowState State(std::uint32_t count,
                                   std::uint32_t index = 0u) {
    pal4::PictureViewWindowState state{};
    state.Write(0x44u, 0x2000u);
    state.Write(0x48u, 0x3000u);
    state.Write(0x4cu, 0x4000u);
    if (count != 0u) {
        state.Write(0x54u, 0xa000u);
        state.Write(0x58u, 0xa000u + count * 4u);
    }
    state.Write(0x60u, index);
    return state;
}

void CheckExistingAndDuplicate() {
    auto state = State(4u);
    Runtime runtime;
    runtime.present = true;
    runtime.words = {{0xa000u, 0x9100u}, {0xa004u, 0x9200u},
                     {0xa008u, 0x9100u}, {0xa00cu, 0x9300u}};
    pal4::PictureViewWindowDisplay::Show(state, 0x9100u, runtime);
    Check(state.Read(0x60u) == 2u && runtime.calls ==
              std::vector<std::string>{"process", "has", "destroy", "record",
                                       "create", "scale", "image", "read", "read",
                                       "read", "read", "right+", "left+", "cleanup"},
          "existing imageset replaced and last duplicate index retained");
}

void CheckEdges() {
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        auto state = State(3u, index);
        Runtime runtime;
        runtime.words = {{0xa000u, 0x9200u}, {0xa004u, 0x9200u},
                         {0xa008u, 0x9200u}};
        pal4::PictureViewWindowDisplay::Show(state, 0x9100u, runtime);
        Check(state.Read(0x60u) == index, "unmatched record retains index");
        const auto n = runtime.calls.size();
        Check(runtime.calls[n - 1u] == "cleanup", "script cleanup is last");
        Check((index == 0u && n >= 4u &&
                   runtime.calls[n - 4u] == "right+" &&
                   runtime.calls[n - 3u] == "left+" &&
                   runtime.calls[n - 2u] == "left-") ||
                  (index == 1u && n >= 3u &&
                   runtime.calls[n - 3u] == "right+" &&
                   runtime.calls[n - 2u] == "left+") ||
                  (index == 2u && n >= 4u &&
                   runtime.calls[n - 4u] == "right+" &&
                   runtime.calls[n - 3u] == "left+" &&
                   runtime.calls[n - 2u] == "right-"),
              "first, middle and last navigation states");
    }

    auto empty = State(0u);
    Runtime runtime;
    pal4::PictureViewWindowDisplay::Show(empty, 0x9100u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "process", "has", "record", "create", "scale", "image",
              "right+", "left+", "left-", "cleanup"},
          "empty array retains right enabled due to unsigned count-minus-one");
}

void CheckException() {
    auto state = State(0u);
    Runtime runtime;
    runtime.fail_create = true;
    try {
        pal4::PictureViewWindowDisplay::Show(state, 0x9100u, runtime);
        Check(false, "create failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "process", "has", "record", "create"},
              "script cleanup is only reached after successful display");
    }
}

}  // namespace

int main() {
    CheckExistingAndDuplicate();
    CheckEdges();
    CheckException();
    std::cout << "Picture view window display tests passed\n";
}
