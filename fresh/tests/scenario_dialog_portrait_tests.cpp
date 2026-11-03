#include "pal4/scenario_dialog.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScenarioPortraitRuntime {
    std::optional<pal4::ScenarioPortraitCacheEntry> cached;
    std::unordered_set<std::string> files;
    std::unordered_set<std::string> present;
    std::unordered_set<std::string> defined;
    std::vector<std::string> events;
    std::vector<std::string> probed;
    std::vector<std::string> created;
    std::vector<pal4::ScenarioPortraitCacheEntry> inserted;
    std::uint32_t image = 0u;
    bool force_missing_image = false;
    std::uint32_t layout_option = 0xffffffffu;

    void BeginResources() override { events.emplace_back("begin"); }
    std::uint32_t EndResources() override {
        events.emplace_back("end");
        return 0x9090u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read");
        if (address == 0x102cu) return 0x2000u;
        Check(address == 0x26b0u, "portrait image field");
        return image;
    }
    void SetImage(std::uint32_t window,
                  std::uint32_t value) override {
        Check(window == 0x2000u, "portrait window");
        events.emplace_back("set-image");
        image = value;
    }
    std::optional<pal4::ScenarioPortraitCacheEntry> FindCache(
        std::uint32_t tree, const std::string& name) override {
        Check(tree == 0x1040u && name == "hero", "cache lookup");
        events.emplace_back("find-cache");
        return cached;
    }
    void InsertCache(
        std::uint32_t tree, const std::string& name,
        const pal4::ScenarioPortraitCacheEntry& entry) override {
        Check(tree == 0x1040u && name == "hero", "cache insert");
        events.emplace_back("insert-cache");
        inserted.push_back(entry);
    }
    bool ImagesetPresent(const std::string& name) override {
        events.emplace_back("present");
        return present.contains(name);
    }
    void CreateImageset(const std::string& file) override {
        events.emplace_back("create");
        created.push_back(file);
    }
    bool PortraitFileExists(const std::string& file) override {
        events.emplace_back("probe");
        probed.push_back(file);
        return files.contains(file);
    }
    bool ImageDefined(const std::string& imageset,
                      const std::string& name) override {
        Check(name == "hero", "defined image name");
        events.emplace_back("defined");
        return defined.contains(imageset);
    }
    std::uint32_t GetImage(const std::string& imageset,
                           const std::string& name) override {
        events.emplace_back("get-image");
        if (imageset == "zhujiemian0" && name == "default")
            return 0xd00du;
        Check(name == "hero", "portrait image name");
        return force_missing_image ? 0u : 0xa000u;
    }
    void SetTextAndImagePositions(
        std::uint32_t object, std::uint32_t option) override {
        Check(object == 0x1000u, "layout owner");
        events.emplace_back("layout");
        layout_option = option;
    }
};

void Run(Runtime& runtime, std::uint32_t option = 0x55u) {
    Check(pal4::ScenarioDialog::SetPortrait(
              0x1000u, "hero", option, runtime) == 0x9090u &&
              runtime.layout_option == option &&
              runtime.events.front() == "begin" &&
              runtime.events.back() == "end" &&
              runtime.events[runtime.events.size() - 2u] == "layout",
          "script and layout boundaries");
}

void CheckCached() {
    Runtime hit;
    hit.cached = pal4::ScenarioPortraitCacheEntry{
        "portrait10", "hero"};
    hit.present.insert("portrait10");
    Run(hit);
    Check(hit.image == 0xa000u && hit.created.empty() &&
              hit.probed.empty() && hit.inserted.empty(),
          "cached image from present imageset");

    Runtime load;
    load.cached = pal4::ScenarioPortraitCacheEntry{
        "portrait20", "hero"};
    Run(load);
    Check(load.created == std::vector<std::string>{
              "gamedata\\ui\\portrait\\portrait2_0.imageset"} &&
              load.image == 0xa000u,
          "cached imageset reconstructs source filename");

    Runtime empty;
    empty.cached = pal4::ScenarioPortraitCacheEntry{"", "hero"};
    Run(empty);
    Check(empty.image == 0xd00du && empty.created.empty() &&
              empty.inserted.empty(),
          "cached empty imageset chooses default");

    Runtime missing;
    missing.cached = pal4::ScenarioPortraitCacheEntry{
        "portrait10", "hero"};
    missing.present.insert("portrait10");
    missing.force_missing_image = true;
    Run(missing);
    Check(missing.image == 0xd00du &&
              missing.inserted.size() == 1u &&
              missing.inserted[0].imageset.empty(),
          "null cached image records default fallback");
}

void CheckScanning() {
    const std::string file0 =
        "gamedata\\ui\\portrait\\portrait0_0.imageset";
    const std::string file1 =
        "gamedata\\ui\\portrait\\portrait1_0.imageset";
    const std::string file2 =
        "gamedata\\ui\\portrait\\portrait2_0.imageset";
    Runtime found;
    found.files.insert(file0);
    found.files.insert(file1);
    found.files.insert(file2);
    found.present.insert("portrait00");
    found.defined.insert("portrait10");
    Run(found, 7u);
    Check(found.probed ==
              (std::vector<std::string>{file0, file1}) &&
              found.created == std::vector<std::string>{file1} &&
              found.inserted.size() == 1u &&
              found.inserted[0].imageset == "portrait10" &&
              found.inserted[0].image == "hero" &&
              found.image == 0xa000u,
          "scan advances through undefined image and caches first hit");

    Runtime absent;
    absent.files.insert(file0);
    Run(absent);
    Check(absent.probed ==
              (std::vector<std::string>{file0, file1}) &&
              absent.inserted.size() == 1u &&
              absent.inserted[0].imageset.empty() &&
              absent.image == 0xd00du,
          "first missing numbered file ends scan and caches default");

    Runtime first_missing;
    Run(first_missing);
    Check(first_missing.probed ==
              std::vector<std::string>{file0} &&
              first_missing.created.empty() &&
              first_missing.image == 0xd00du,
          "zero-file scan falls back immediately");
}

}  // namespace

int main() {
    try {
        CheckCached();
        CheckScanning();
        std::cout << "scenario_dialog_portrait_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
