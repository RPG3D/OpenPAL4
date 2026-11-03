#include "pal4/system_setting_frame_mode.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingFrameModeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::pair<std::uint32_t, bool>> selection;
    std::tuple<std::uint32_t, std::string, std::string> image;
    std::vector<std::string> calls;
    std::uint32_t manager_count = 0u;
    std::uint32_t end_count = 0u;
    bool fail_find = false;
    bool data_mode = false;
    std::int32_t populated_page = 0;

    Runtime() {
        for (std::uint32_t i = 0; i < 3u; ++i) {
            words[0x4000u + 112u + 4u * i] = 0x1000u + i;
            words[0x4000u + 124u + 4u * i] = 0x2000u + i;
        }
        words[0x4000u + 496u] = 0x3000u;
        words[0x6000u + 44u] = 27u;
        words[0x4000u + 180u] = 0x4000u;
        for (std::uint32_t i = 0; i < 4u; ++i) {
            words[0x4000u + 184u + 4u * i] = 0x4100u + i;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back("visible");
        visibility.emplace_back(window, visible);
    }
    void SetSelected(std::uint32_t button, bool selected) override {
        calls.emplace_back("selected");
        selection.emplace_back(button, selected);
    }
    void SetImage(std::uint32_t window, std::string_view name,
                  std::string_view imageset) override {
        calls.emplace_back("image");
        image = {window, std::string(name), std::string(imageset)};
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        ++manager_count;
        return 0x8000u;
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x6000u;
    }
    void PopulateSaveList(std::uint32_t frame,
                          std::int32_t page, std::uint32_t mode) override {
        calls.emplace_back("populate");
        Check(frame == 0x4000u && mode == 1u,
              "save list frame and mode");
        populated_page = page;
    }
    void RemoveActive(std::uint32_t collection,
                      std::uint32_t member) override {
        calls.emplace_back("remove");
        Check(collection == 0x8000u + 308u &&
              member == (data_mode ? 0x4040u : 0x4060u),
              "remove active member");
    }
    void InsertActive(std::uint32_t collection,
                      std::uint32_t member) override {
        calls.emplace_back("insert");
        Check(collection == 0x8000u + 308u &&
              member == (data_mode ? 0x4060u : 0x4040u),
              "insert active member");
    }
    std::uint32_t BeginName(std::uint32_t frame) override {
        calls.emplace_back("begin");
        Check(frame == 0x4000u, "name frame");
        return 0x7000u;
    }
    std::uint32_t ResetName(std::uint32_t buffer,
                            std::uint32_t frame,
                            std::string_view suffix) override {
        calls.emplace_back("reset");
        Check(buffer == 0x7000u && frame == 0x4000u &&
              suffix == "Savewindowblanknew", "blank resource spelling");
        return 0x7100u;
    }
    std::uint32_t FindWindow(std::uint32_t name) override {
        calls.emplace_back("find");
        Check(name == 0x7100u, "blank resource lookup");
        if (fail_find) throw std::runtime_error("lookup");
        return 0x5000u;
    }
    std::uint32_t EndName(std::uint32_t buffer) override {
        calls.emplace_back("end");
        Check(buffer == 0x7000u, "name release buffer");
        ++end_count;
        return 0x55u;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::SystemSettingFrameMode::ShowSave(0x4000u, runtime) == 0x55u,
          "returns name release result");
    Check(runtime.visibility == std::vector<std::pair<std::uint32_t, bool>>{
          {0x1000u, true}, {0x1001u, true}, {0x1002u, true},
          {0x2000u, false}, {0x2001u, false}, {0x2002u, false},
          {0x5000u, true}}, "window visibility order");
    Check(runtime.words.at(0x405cu) == 0u &&
          runtime.words.at(0x4058u) == 0x3000u &&
          runtime.manager_count == 2u && runtime.end_count == 1u,
          "slider reference and manager accesses");
    Check(runtime.calls == std::vector<std::string>{
          "read", "visible", "read", "visible", "read", "visible",
          "read", "visible", "read", "visible", "read", "visible",
          "read", "write", "write", "manager", "remove", "manager",
          "insert", "begin", "reset", "find", "visible", "end"},
          "mode transition call order");

    Runtime failure;
    failure.fail_find = true;
    bool caught = false;
    try {
        (void)pal4::SystemSettingFrameMode::ShowSave(0x4000u, failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.end_count == 1u &&
          failure.visibility.size() == 6u,
          "name cleanup and prior state on lookup failure");

    Runtime data;
    data.data_mode = true;
    Check(pal4::SystemSettingFrameMode::ShowDataMode(0x4000u, data) == 0x55u &&
          data.populated_page == 2 && data.manager_count == 2u &&
          data.end_count == 1u, "data mode page and result");
    Check(data.visibility == std::vector<std::pair<std::uint32_t, bool>>{
          {0x1000u, false}, {0x1001u, false}, {0x1002u, false},
          {0x2000u, true}, {0x2001u, true}, {0x2002u, true},
          {0x5000u, true}}, "data mode visibility order");
    Check(data.calls == std::vector<std::string>{
          "read", "visible", "read", "visible", "read", "visible",
          "read", "visible", "read", "visible", "read", "visible",
          "global", "read", "populate", "manager", "remove",
          "manager", "insert", "begin", "reset", "find", "visible", "end"},
          "data mode call order");

    Runtime negative;
    negative.data_mode = true;
    negative.words[0x6000u + 44u] = static_cast<std::uint32_t>(-27);
    (void)pal4::SystemSettingFrameMode::ShowDataMode(0x4000u, negative);
    Check(negative.populated_page == -2, "signed page division toward zero");

    Runtime system;
    Check(pal4::SystemSettingFrameMode::ShowSystem(0x4000u, system) == 0x55u &&
          system.end_count == 1u && system.manager_count == 0u,
          "system mode result and no manager switch");
    Check(system.visibility == std::vector<std::pair<std::uint32_t, bool>>{
          {0x1000u, false}, {0x1001u, false}, {0x1002u, false},
          {0x2000u, false}, {0x2001u, false}, {0x2002u, false},
          {0x5000u, false}}, "system mode hides both groups and blank");
    Check(system.selection == std::vector<std::pair<std::uint32_t, bool>>{
          {0x4100u, false}, {0x4101u, false},
          {0x4102u, false}, {0x4103u, false}},
          "system mode clears all four selections");
    Check(system.image == std::tuple{0x4000u,
                                    std::string("zi0"),
                                    std::string("xitong")},
          "system mode image and imageset");
    Check(system.calls == std::vector<std::string>{
          "read", "visible", "read", "visible", "read", "visible",
          "read", "visible", "read", "visible", "read", "visible",
          "read", "selected", "read", "selected", "read", "selected",
          "read", "selected", "read", "image", "begin", "reset",
          "find", "visible", "end"}, "system mode call order");
}
