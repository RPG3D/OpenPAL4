#include "pal4/mission_window_quest_image.h"

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

struct Runtime final : pal4::IMissionWindowQuestImageRuntime {
    std::vector<std::string> calls;
    std::uint32_t flag{1u};
    bool present{};
    bool fail_create{};
    bool fail_image{};

    std::uint32_t ImageFlag(std::uint32_t quest,
                            std::uint32_t offset) override {
        Check(quest == 0x7000u && offset == 0x2cu,
              "quest image flag field");
        calls.emplace_back("flag");
        return flag;
    }
    std::string ImageKey(std::uint32_t quest,
                          std::uint32_t offset) override {
        Check(quest == 0x7000u && offset == 0x28u,
              "quest image key field");
        calls.emplace_back("key");
        return "hero";
    }
    void PreparePkFileManager() override {
        calls.emplace_back("prepare");
    }
    void CleanupPkFileManager() override {
        calls.emplace_back("cleanup");
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("convert:" + std::string(text));
        return std::string(text);
    }
    bool ImagesetPresent(std::string_view name) override {
        Check(name == "Pal4QuestImagehero", "imageset name");
        calls.emplace_back("present");
        return present;
    }
    void CreateImagesetFromImageFile(
        std::string_view name, std::string_view path,
        std::string_view resource_group) override {
        Check(name == "Pal4QuestImagehero" &&
                  path == "gamedata\\ui\\quest\\hero.png" &&
                  resource_group.empty(), "imageset creation arguments");
        calls.emplace_back("create");
        if (fail_create) throw std::runtime_error("create");
    }
    std::uint32_t GetImageset(std::string_view name) override {
        Check(name == "Pal4QuestImagehero", "get imageset by name");
        calls.emplace_back("get-set");
        return 0x8000u;
    }
    void SetAutoScalingEnabled(std::uint32_t imageset,
                               bool enabled) override {
        Check(imageset == 0x8000u && !enabled,
              "quest image disables auto scaling");
        calls.emplace_back("autoscale");
    }
    std::uint32_t GetImage(std::uint32_t imageset,
                            std::string_view name) override {
        Check(imageset == 0x8000u && name == "full_image",
              "full image lookup");
        calls.emplace_back("image");
        if (fail_image) throw std::runtime_error("image");
        return 0x9000u;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::MissionWindowQuestImage::Load(0u, runtime) == 0u &&
              runtime.calls.empty(),
          "null quest returns without runtime calls");
    runtime.flag = 0u;
    Check(pal4::MissionWindowQuestImage::Load(0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"flag"},
          "disabled image returns before PK manager work");

    runtime = Runtime{};
    Check(pal4::MissionWindowQuestImage::Load(0x7000u, runtime) ==
              0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "flag", "prepare", "key",
                  "convert:gamedata\\ui\\quest\\hero.png",
                  "convert:Pal4QuestImagehero", "present", "create",
                  "get-set", "autoscale", "convert:full_image",
                  "cleanup", "image"},
          "missing imageset is created before full image lookup");

    runtime = Runtime{};
    runtime.present = true;
    Check(pal4::MissionWindowQuestImage::Load(0x7000u, runtime) ==
              0x9000u &&
              runtime.calls == std::vector<std::string>{
                  "flag", "prepare", "key",
                  "convert:gamedata\\ui\\quest\\hero.png",
                  "convert:Pal4QuestImagehero", "present", "get-set",
                  "autoscale", "convert:full_image", "cleanup", "image"},
          "present imageset skips creation");

    runtime = Runtime{};
    runtime.fail_create = true;
    try {
        (void)pal4::MissionWindowQuestImage::Load(0x7000u, runtime);
        Check(false, "creation failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "cleanup",
              "creation failure cleans PK manager");
    }
    runtime = Runtime{};
    runtime.fail_image = true;
    try {
        (void)pal4::MissionWindowQuestImage::Load(0x7000u, runtime);
        Check(false, "image failure propagates");
    } catch (const std::runtime_error&) {
        std::size_t cleanup_count{};
        for (const auto& call : runtime.calls)
            if (call == "cleanup") ++cleanup_count;
        Check(cleanup_count == 1u && runtime.calls.back() == "image",
              "image failure does not repeat prior cleanup");
    }
    std::cout << "Mission window quest image tests passed\n";
}
