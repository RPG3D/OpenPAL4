#include "pal4/mission_window_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Subscription {
    std::uint32_t window;
    pal4::MissionWindowEvent event;
    std::uint32_t handler;
    std::uint32_t owner;
    std::uint32_t bytes;
    std::uint32_t vtable;

    bool operator==(const Subscription&) const = default;
};

struct Runtime final : pal4::IMissionWindowLifecycleRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::string, bool>> lookups;
    std::vector<Subscription> subscriptions;
    std::uint32_t columns{2u};
    std::size_t throw_at_lookup{100u};
    bool throw_at_title{};

    void LoadLayout(pal4::MissionWindowState& state, std::uint32_t name,
                    std::uint32_t first, std::uint32_t second) override {
        Check(name == 0x7000u && first == 0u && second == 0u,
              "layout arguments");
        state.Write(0x24u, 0x5000u);
        calls.emplace_back("load");
    }
    void InitializeMember(pal4::MissionWindowState& state) override {
        Check(state.Read(0x24u) == 0x5000u, "member follows layout");
        calls.emplace_back("member");
    }
    std::uint32_t ObjectAddress(const pal4::MissionWindowState& state) override {
        Check(state.Read(0u) == 0x8439ccu &&
                  state.Read(0x28u) == 0x8439a4u &&
                  state.Read(0x40u) == 0x8439fcu &&
                  state.Read(0x48u) == 0x8439e4u &&
                  state.bytes[0x88u] == 1u,
              "constructor installs object and member state");
        calls.emplace_back("owner");
        return 0x6000u;
    }
    std::uint32_t FindWindow(std::string_view path, bool by_name) override {
        if (lookups.size() == throw_at_lookup)
            throw std::runtime_error("lookup");
        lookups.emplace_back(path, by_name);
        calls.emplace_back("lookup");
        return 0x1000u + static_cast<std::uint32_t>(lookups.size());
    }
    void SetAlwaysOnTop(std::uint32_t window, bool value) override {
        Check(window == 0x100du && value, "mask always on top");
        calls.emplace_back("top");
    }
    void Subscribe(std::uint32_t window, pal4::MissionWindowEvent event,
                   std::uint32_t handler, std::uint32_t owner,
                   std::uint32_t bytes, std::uint32_t vtable) override {
        subscriptions.push_back({window, event, handler, owner, bytes, vtable});
        calls.emplace_back("subscribe");
    }
    void SetUserColumnSizing(std::uint32_t list, bool value) override {
        Check((list == 0x1001u || list == 0x1002u) && !value,
              "column sizing disabled");
        calls.emplace_back("sizing");
    }
    void SetUserColumnDragging(std::uint32_t list, bool value) override {
        Check((list == 0x1001u || list == 0x1002u) && !value,
              "column dragging disabled");
        calls.emplace_back("dragging");
    }
    void SetUserSortControl(std::uint32_t list, bool value) override {
        Check((list == 0x1001u || list == 0x1002u) && !value,
              "sort control disabled");
        calls.emplace_back("sorting");
    }
    void SetListHeaderByte(std::uint32_t list, std::uint32_t offset,
                           std::uint8_t value) override {
        Check(list == 0x1002u && offset == 1239u && value == 0u,
              "story list header byte");
        calls.emplace_back("header");
    }
    std::uint32_t ColumnCount(std::uint32_t list) override {
        Check(list == 0x1002u, "story column count target");
        calls.emplace_back("count");
        return columns;
    }
    void RemoveColumn(std::uint32_t list, std::uint32_t index) override {
        Check(list == 0x1002u && index == 0u && columns != 0u,
              "remove first story column");
        --columns;
        calls.emplace_back("remove");
    }
    void ResetList(std::uint32_t list) override {
        Check(list == 0x1002u && columns == 0u, "reset cleared story list");
        calls.emplace_back("reset");
    }
    std::string TriggerTitle(std::uint32_t trigger_id) override {
        Check(trigger_id == 193u, "title trigger ID");
        calls.emplace_back("title");
        if (throw_at_title) throw std::runtime_error("title");
        return "legacy title";
    }
    std::string ConvertToUtf8(std::string_view title) override {
        Check(title == "legacy title", "source title conversion");
        calls.emplace_back("utf8");
        return "UTF-8 title";
    }
    void AddColumn(std::uint32_t list, std::string_view title,
                   std::uint32_t id, float width) override {
        Check(list == 0x1002u && title == "UTF-8 title" && id == 0u &&
                  width == 0.89999998f, "story column");
        calls.emplace_back("add");
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        Check(window == 0x1008u && !value,
              "edit description disabled");
        calls.emplace_back("enabled");
    }
    void SetupMouseHandlers(pal4::MissionWindowState& state,
                            std::uint32_t button) override {
        Check(state.Read(0x28u) == 0x8439a4u &&
                  (button == 0x1009u || button == 0x100au),
              "button mouse handler target");
        calls.emplace_back("mouse");
    }
    void DestroyMember(pal4::MissionWindowState&) override {
        calls.emplace_back("destroy-member");
    }
    void DestroyDialogBase(pal4::MissionWindowState&) override {
        calls.emplace_back("destroy-base");
    }
};

void TestSuccess() {
    pal4::MissionWindowState state{};
    Runtime runtime;
    Check(&pal4::MissionWindowLifecycle::Construct(state, 0x7000u, runtime) ==
              &state, "constructor returns state");
    const std::vector<std::pair<std::string, bool>> expected{
        {"MissionWindow/List", true},
        {"MissionWindow/ListStory", false},
        {"MissionWindow/ImgMission", true},
        {"MissionWindow/PanelPicture", false},
        {"MissionWindow/PanelDesc", false},
        {"MissionWindow/PanelScenario", true},
        {"MissionWindow/ListWindow", true},
        {"MissionWindow/EditDescription", true},
        {"MissionWindow/BtnMission", true},
        {"MissionWindow/BtnScenario", true},
        {"MissionWindow/zj", false},
        {"MissionWindow/ImageLabel", false},
        {"MissionWindow/ImgDisusedMask", false},
    };
    Check(runtime.lookups == expected, "exact lookup paths and methods");
    const std::vector<std::uint32_t> offsets{
        0x54u, 0x58u, 0x68u, 0x70u, 0x78u, 0x6cu, 0x74u,
        0x5cu, 0x60u, 0x64u, 0x7cu, 0x80u, 0x84u};
    for (std::size_t i = 0; i < offsets.size(); ++i)
        Check(state.Read(offsets[i]) == 0x1001u + i,
              "window stored at verified offset");
    Check(state.Read(0x44u) == 0x6000u &&
              state.Read(0x4cu) == 0x6000u &&
              state.Read(0x50u) == 0x1002u,
          "final member links");
    const auto click = pal4::MissionWindowEvent::Clicked;
    const auto hover = pal4::MissionWindowEvent::HoverRowChanged;
    const auto mouse = pal4::MissionWindowEvent::MouseClick;
    const std::vector<Subscription> expected_subscriptions{
        {0x1009u, click, 0x46fdd0u, 0x6000u, 24u, 0x84399cu},
        {0x100au, click, 0x46fe10u, 0x6000u, 24u, 0x84399cu},
        {0x1001u, hover, 0x46fe60u, 0x6000u, 24u, 0x84399cu},
        {0x1001u, mouse, 0x46fe70u, 0x6000u, 24u, 0x84399cu},
        {0x1002u, mouse, 0x46ffa0u, 0x6000u, 24u, 0x84399cu},
    };
    Check(runtime.subscriptions == expected_subscriptions,
          "callback targets and payloads");
    Check(runtime.columns == 0u, "all old story columns removed");
    const std::vector<std::string> expected_tail{
        "top", "owner", "subscribe", "subscribe", "subscribe",
        "subscribe", "sizing", "dragging", "sorting", "header",
        "sizing", "dragging", "sorting", "count", "remove",
        "count", "remove", "count", "reset", "title", "utf8",
        "add", "header", "subscribe", "enabled", "mouse", "mouse"};
    Check(std::vector<std::string>(runtime.calls.begin() + 15,
                                   runtime.calls.end()) == expected_tail,
          "post-lookup operation order");
}

void TestFailure(std::size_t lookup, bool title) {
    pal4::MissionWindowState state{};
    Runtime runtime;
    runtime.throw_at_lookup = lookup;
    runtime.throw_at_title = title;
    try {
        (void)pal4::MissionWindowLifecycle::Construct(state, 0x7000u,
                                                       runtime);
        Check(false, "failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.size() >= 2u &&
                  runtime.calls[runtime.calls.size() - 2u] ==
                      "destroy-member" &&
                  runtime.calls.back() == "destroy-base",
              "member and base cleaned on failure");
    }
}

}  // namespace

int main() {
    TestSuccess();
    TestFailure(0u, false);
    TestFailure(7u, false);
    TestFailure(100u, true);
    std::cout << "Mission window lifecycle tests passed\n";
}
