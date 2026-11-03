#include "pal4/picture_view_window_commands.h"

#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::PictureViewWindowCommandsRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    bool imageset{};
    std::uint32_t preview{0x7000u};

    void SetFrameVisible(std::string_view name, bool visible) override {
        Check(name == "PictureViewWindow", "picture window name");
        calls.emplace_back(visible ? "show" : "hide-frame");
    }
    void Dispatch(std::string_view command, std::uint32_t first,
                  std::uint32_t second) override {
        Check(command == "show_picture_preview" && first == 1u &&
                  second == 0u, "preview event arguments");
        calls.emplace_back("preview-event");
    }
    std::uint32_t FindFrame(std::string_view name) override {
        calls.emplace_back(std::string("find:") + std::string(name));
        return name == "PictureViewWindow" ? 0x6000u : preview;
    }
    void Display(std::uint32_t frame, std::uint32_t record) override {
        Check(frame == 0x6000u, "display targets picture frame");
        calls.emplace_back("display:" + std::to_string(record));
    }
    bool HasImageset(std::string_view name) override {
        Check(name == "ui_picture_view_imageset", "imageset name");
        calls.emplace_back("has");
        return imageset;
    }
    void DestroyImageset(std::string_view name) override {
        Check(name == "ui_picture_view_imageset", "destroy name");
        calls.emplace_back("destroy");
    }
    void Hide(pal4::PictureViewWindowState&, std::uint32_t first,
              std::uint32_t second) override {
        Check(first == 0u && second == 0u, "hide virtual arguments");
        calls.emplace_back("hide");
    }
    void UpdatePreview(std::uint32_t frame, std::uint32_t index) override {
        Check(frame == preview && index == 2u, "preview restores index");
        calls.emplace_back("update-preview");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t HandleOtherMemberEvent(std::uint32_t member,
                                         std::uint32_t event) override {
        Check(member == 0x6028u && event == 0x9000u,
              "other member event forwarded");
        calls.emplace_back("base-event");
        return 7u;
    }
};

pal4::PictureViewWindowState State() {
    pal4::PictureViewWindowState state{};
    state.Write(0x54u, 0xa000u);
    state.Write(0x58u, 0xa00cu);
    state.Write(0x60u, 2u);
    return state;
}

void CheckViewAndClose() {
    using Commands = pal4::PictureViewWindowCommands;
    Runtime runtime;
    Check(Commands::HandleViewCommand(1u, 0x9100u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "show", "find:PictureViewWindow", "display:37120"},
          "show command finds frame then displays record");
    runtime.calls.clear();
    Check(Commands::HandleViewCommand(0u, 0x9100u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "hide-frame", "preview-event"},
          "hide command restores preview");

    auto state = State();
    runtime.calls.clear();
    runtime.imageset = true;
    Check(Commands::Close(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "has", "destroy", "hide", "preview-event",
                  "find:picturePreviewWindow", "update-preview"},
          "close removes imageset, hides, then updates preview");
    runtime.calls.clear();
    runtime.imageset = false;
    runtime.preview = 0u;
    Check(Commands::Close(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "has", "hide", "preview-event", "find:picturePreviewWindow"},
          "absent imageset and preview skip optional actions");
}

void CheckNavigation() {
    using Commands = pal4::PictureViewWindowCommands;
    auto state = State();
    Runtime runtime;
    runtime.words = {{0xa000u, 101u}, {0xa004u, 102u},
                     {0xa008u, 103u}};
    Check(Commands::Previous(state, 0x6000u, runtime) == 1u &&
              state.Read(0x60u) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "read:40964", "display:102"},
          "previous decrements and displays indexed record");
    runtime.calls.clear();
    Check(Commands::Next(state, 0x6000u, runtime) == 1u &&
              state.Read(0x60u) == 2u &&
              runtime.calls == std::vector<std::string>{
                  "read:40968", "display:103"},
          "next increments and displays indexed record");
    runtime.calls.clear();
    Check(Commands::Next(state, 0x6000u, runtime) == 1u &&
              state.Read(0x60u) == 2u &&
              runtime.calls == std::vector<std::string>{
                  "read:40968", "display:103"},
          "next clamps at last record");

    state = {};
    runtime.calls.clear();
    Check(Commands::Next(state, 0x6000u, runtime) == 1u &&
              state.Read(0x60u) == 0xffffffffu &&
              runtime.calls.empty(), "empty next leaves signed minus-one index");
    Check(Commands::Previous(state, 0x6000u, runtime) == 1u &&
              state.Read(0x60u) == 0u && runtime.calls.empty(),
          "empty previous clamps to zero");
}

void CheckMemberEvent() {
    auto state = State();
    Runtime runtime;
    Check(pal4::PictureViewWindowCommands::OnCloseMemberEvent(
              state, 0x6028u, 2u, 0x9000u, runtime) == 7u &&
              runtime.calls == std::vector<std::string>{"base-event"},
          "other member events forward to base");
    runtime.calls.clear();
    Check(pal4::PictureViewWindowCommands::OnCloseMemberEvent(
              state, 0x6028u, 1u, 0x9000u, runtime) == 1u &&
              runtime.calls.front() == "has" &&
              runtime.calls.back() == "update-preview",
          "event one invokes close command");
}

}  // namespace

int main() {
    CheckViewAndClose();
    CheckNavigation();
    CheckMemberEvent();
    std::cout << "Picture view window command tests passed\n";
}
