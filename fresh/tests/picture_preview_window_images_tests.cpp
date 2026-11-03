#include "pal4/picture_preview_window_images.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PicturePreviewWindowImagesRuntimeStub {
    std::vector<std::uint32_t> indices;
    std::vector<std::string> calls;
    bool all_missing{};
    std::uint32_t throw_index{0xffffffffu};

    bool HasTriggerTask(std::uint32_t index) override {
        indices.push_back(index);
        calls.emplace_back("has");
        if (index == throw_index) throw std::runtime_error("task lookup");
        return !all_missing && index == 100u;
    }
    std::uint32_t TriggerTaskAddress(std::uint32_t index) override {
        Check(index == 100u, "only first task exists");
        calls.emplace_back("task");
        return 0x7000u;
    }
    std::uint32_t TriggerTaskId(std::uint32_t task) override {
        Check(task == 0x7000u, "image task ID source");
        calls.emplace_back("id");
        return 0x123u;
    }
    std::string TriggerTaskImageName(std::uint32_t task) override {
        Check(task == 0x7000u, "image name source");
        calls.emplace_back("name");
        return "raw";
    }
    std::string ConvertToUtf8(std::string_view raw) override {
        Check(raw == "raw", "image name conversion source");
        calls.emplace_back("utf8");
        return std::string("image-name\0ignored", 18u);
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check(window >= 0x2100u && window <= 0x2a00u,
              "image enable target");
        calls.emplace_back(enabled ? "enable" : "disable");
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x2100u && visible,
              "existing first task image becomes visible");
        calls.emplace_back("visible");
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(window == 0x2100u && imageset == "biaoti20" &&
                  image == "image-name", "image uses title set and UTF-8 name");
        calls.emplace_back("image");
    }
    void SetImagePointer(std::uint32_t window,
                         std::uint32_t image) override {
        Check(window >= 0x2100u && window <= 0x2a00u &&
                  image == 0xd000u, "missing task uses saved disabled image");
        calls.emplace_back("fallback");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x2218u && value == 0x123u,
              "first image receives task ID at +0x118");
        calls.emplace_back("write");
    }
};

pal4::PicturePreviewWindowState State() {
    pal4::PicturePreviewWindowState state{};
    for (std::uint32_t i = 0u; i < 10u; ++i)
        state.Write(0x2cu + i * 4u, 0x2100u + i * 0x100u);
    state.Write(0x60u, 0xd000u);
    return state;
}

void CheckMixedTasks() {
    auto state = State();
    Runtime runtime;
    Check(pal4::PicturePreviewWindowImages::Update(state, runtime) == 10u &&
              runtime.indices.size() == 10u &&
              runtime.indices.front() == 100u &&
              runtime.indices.back() == 109u,
          "ten consecutive task numbers processed");
    Check(runtime.calls == std::vector<std::string>{
              "has", "task", "visible", "enable", "name", "utf8",
              "image", "id", "write", "has", "fallback", "disable",
              "has", "fallback", "disable", "has", "fallback", "disable",
              "has", "fallback", "disable", "has", "fallback", "disable",
              "has", "fallback", "disable", "has", "fallback", "disable",
              "has", "fallback", "disable", "has", "fallback", "disable"},
          "existing and missing task calls preserve IDA order");
}

void CheckFailure() {
    auto state = State();
    Runtime runtime;
    runtime.all_missing = true;
    runtime.throw_index = 101u;
    try {
        (void)pal4::PicturePreviewWindowImages::Update(state, runtime);
        Check(false, "task lookup error must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.indices == std::vector<std::uint32_t>{100u, 101u} &&
                  runtime.calls == std::vector<std::string>{
                      "has", "fallback", "disable", "has"},
              "failure stops after first missing image");
    }
}

}  // namespace

int main() {
    CheckMixedTasks();
    CheckFailure();
    std::cout << "Picture preview window image tests passed\n";
}
