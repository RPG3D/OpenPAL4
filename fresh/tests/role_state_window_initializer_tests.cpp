#include "pal4/role_state_window_initializer.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Subscription {
    std::uint32_t window;
    pal4::RoleMouseEvent event;
    std::uint32_t handler;
    std::uint32_t owner;
    bool factory;
};

struct Runtime final : pal4::IRoleStateWindowInitRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::vector<std::string> patterns;
    std::vector<std::int32_t> indices;
    std::vector<std::uint32_t> tagged;
    std::vector<Subscription> subscriptions;
    std::uint32_t image_count = 0u;
    std::uint32_t window_count = 0u;
    std::uint32_t cleanup_count = 0u;
    int throw_on_window = -1;
    bool throw_on_image = false;

    std::uint32_t FindImage(std::string_view set,
                            std::string_view image) override {
        static constexpr std::string_view expected[]{
            "tianhe1", "hanlinsha1", "liumenli1", "ziying1"};
        Check(set == "kuang0" && image_count < 4u &&
                  image == expected[image_count],
              "portrait resource order and names");
        calls.emplace_back("image");
        if (throw_on_image) throw std::runtime_error("image");
        return 0x6000u + image_count++;
    }
    void InitializeNameBuffer() override {
        calls.emplace_back("init-name");
    }
    std::string FormatName(std::string_view pattern,
                           std::int32_t index) override {
        patterns.emplace_back(pattern);
        indices.push_back(index);
        char buffer[96]{};
        std::snprintf(buffer, sizeof(buffer), std::string(pattern).c_str(),
                      index);
        calls.emplace_back("format");
        return buffer;
    }
    std::uint32_t FindWindow(std::string_view name) override {
        if (static_cast<int>(window_count) == throw_on_window)
            throw std::runtime_error("window");
        names.emplace_back(name);
        calls.emplace_back("window");
        return 0x5000u + window_count++ * 0x10u;
    }
    void SetRoleIndex(std::uint32_t window,
                      std::uint32_t index) override {
        Check(index == expected_role_index, "full role index tag");
        tagged.push_back(window);
        calls.emplace_back("tag");
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        Check(window == 0x5010u && !enabled, "role name disabled");
        calls.emplace_back("disable");
    }
    void SetFont(std::uint32_t window,
                 std::string_view font) override {
        Check(window == 0x5020u && font == "Level_font",
              "role level font");
        calls.emplace_back("font");
    }
    void SetTextFormatting(std::uint32_t window,
                           std::uint32_t horizontal,
                           std::uint32_t vertical) override {
        Check(window == 0x5020u && horizontal == 2u && vertical == 2u,
              "role level text formatting");
        calls.emplace_back("formatting");
    }
    std::uint32_t WidthBits(std::uint32_t window) override {
        Check(window >= 0x50b0u && window <= 0x50d0u &&
                  (window - 0x50b0u) % 0x10u == 0u,
              "width reads HP, MP, Rage in order");
        calls.emplace_back("width");
        return 0x3f000000u + window;
    }
    void Subscribe(std::uint32_t window,
                   pal4::RoleMouseEvent event,
                   std::uint32_t handler, std::uint32_t owner,
                   bool factory) override {
        subscriptions.push_back({window, event, handler, owner, factory});
        calls.emplace_back("subscribe");
    }
    std::uint32_t CleanupNameBuffer() override {
        ++cleanup_count;
        calls.emplace_back("cleanup-name");
        return 0xabcu;
    }

    std::uint32_t expected_role_index = 1u;
};

void CheckBindings(const pal4::RoleStateWindowState& state,
                   const Runtime& runtime, std::uint32_t display_index,
                   std::int32_t narrowed_index) {
    static constexpr std::uint32_t offsets[]{
        12u, 100u, 20u, 24u, 28u, 32u, 36u, 40u,
        48u, 52u, 56u, 60u, 64u, 68u, 72u, 80u,
        84u, 88u, 92u, 96u, 76u};
    static constexpr std::string_view formats[]{
        "roleStateWindow/RolePanel%d", "roleStateWindow/RoleName%d",
        "roleStateWindow/RoleLevel%d", "roleStateWindow/RoleWater%d",
        "roleStateWindow/RoleFire%d", "roleStateWindow/RoleThunder%d",
        "roleStateWindow/RoleWind%d", "roleStateWindow/RoleDust%d",
        "roleStateWindow/jing%d", "roleStateWindow/shen%d",
        "roleStateWindow/qi%d", "roleStateWindow/ImgHp%d",
        "roleStateWindow/ImgMp%d", "roleStateWindow/ImgRage%d",
        "roleStateWindow/xiakuang%d", "roleStateWindow/RoleInfo%d_1",
        "roleStateWindow/RoleInfo%d_2", "roleStateWindow/RoleInfo%d_3",
        "roleStateWindow/RoleInfo%d_4", "roleStateWindow/baise%d",
        "roleStateWindow/ImgCurtain%d"};
    Check(runtime.names.size() == 21u && runtime.patterns.size() == 21u,
          "all 21 controls are bound");
    for (std::size_t i = 0; i < 21u; ++i) {
        const auto expected_index =
            i == 0u || i == 2u || i == 3u
                ? static_cast<std::int32_t>(display_index)
                : narrowed_index;
        Check(runtime.patterns[i] == formats[i] &&
                  runtime.indices[i] == expected_index &&
                  state.Read(offsets[i]) == 0x5000u + i * 0x10u,
              "control name, index width and destination offset");
    }
}

void CheckSubscriptions(const Runtime& runtime) {
    Check(runtime.subscriptions.size() == 4u, "four mouse subscriptions");
    const auto& subscriptions = runtime.subscriptions;
    Check(subscriptions[0].window == 0x5000u &&
              subscriptions[0].event == pal4::RoleMouseEvent::Move &&
              subscriptions[0].handler == 0x476910u &&
              !subscriptions[0].factory &&
              subscriptions[1].window == 0x5130u &&
              subscriptions[1].event == pal4::RoleMouseEvent::Move &&
              subscriptions[1].handler == 0x476910u &&
              !subscriptions[1].factory &&
              subscriptions[2].window == 0x5000u &&
              subscriptions[2].event == pal4::RoleMouseEvent::Leave &&
              subscriptions[2].handler == 0x476970u &&
              !subscriptions[2].factory &&
              subscriptions[3].window == 0x5130u &&
              subscriptions[3].event == pal4::RoleMouseEvent::Leave &&
              subscriptions[3].handler == 0x476970u &&
              subscriptions[3].factory,
          "mouse handlers, targets and final callback factory");
    for (const auto& subscription : subscriptions)
        Check(subscription.owner == 0xa000u, "callback owner");
}

void TestNormalAndNarrowedRoleIndex() {
    Runtime runtime;
    pal4::RoleStateWindowState state;
    state.words.fill(0xdeadbeefu);
    Check(pal4::RoleStateWindowInitializer::Initialize(
              state, 0xa000u, 0u, 0x1234u, 0x4321u, runtime) == 0xabcu,
          "returns final string cleanup result");
    Check(state.Read(4u) == 0u && state.Read(8u) == 0u &&
              state.Read(116u) == 0x4321u &&
              state.Read(0u) == 0xdeadbeefu,
          "initial fields and untouched first word");
    for (std::uint32_t i = 0u; i < 4u; ++i)
        Check(state.Read(120u + i * 4u) == 0x6000u + i,
              "portrait image destination");
    CheckBindings(state, runtime, 1u, 1);
    Check(runtime.tagged == std::vector<std::uint32_t>{
              0x5000u, 0x5130u, 0x5140u},
          "panel, white background and curtain tagged");
    for (std::uint32_t i = 0u; i < 3u; ++i)
        Check(state.Read(104u + i * 4u) ==
                  0x3f000000u + 0x50b0u + i * 0x10u,
              "HP, MP and Rage widths cached");
    CheckSubscriptions(runtime);
    Check(runtime.cleanup_count == 1u &&
              runtime.calls.back() == "cleanup-name",
          "name buffer cleaned after subscriptions");

    Runtime large;
    large.expected_role_index = 385u;
    pal4::RoleStateWindowState other;
    static_cast<void>(pal4::RoleStateWindowInitializer::Initialize(
        other, 0xa000u, 384u, 0u, 7u, large));
    CheckBindings(other, large, 385u, -127);
    Check(large.tagged.size() == 3u,
          "window tags keep full index despite name narrowing");
}

void TestUnwind() {
    Runtime window_failure;
    window_failure.throw_on_window = 5;
    pal4::RoleStateWindowState state;
    try {
        static_cast<void>(pal4::RoleStateWindowInitializer::Initialize(
            state, 0xa000u, 0u, 0u, 0u, window_failure));
        Check(false, "window lookup should throw");
    } catch (const std::runtime_error&) {
        Check(window_failure.cleanup_count == 1u &&
                  window_failure.subscriptions.empty(),
              "name buffer unwound after window failure");
    }

    Runtime image_failure;
    image_failure.throw_on_image = true;
    try {
        static_cast<void>(pal4::RoleStateWindowInitializer::Initialize(
            state, 0xa000u, 0u, 0u, 0u, image_failure));
        Check(false, "image lookup should throw");
    } catch (const std::runtime_error&) {
        Check(image_failure.cleanup_count == 0u &&
                  image_failure.names.empty(),
              "image failure occurs before name buffer construction");
    }
}

}  // namespace

int main() {
    TestNormalAndNarrowedRoleIndex();
    TestUnwind();
}
