#include "pal4/role_state_window_constructor.h"

#include <cstdlib>
#include <iostream>
#include <map>
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

struct InitRuntime final : pal4::IRoleStateWindowInitRuntime {
    std::vector<std::string>& order;
    std::uint32_t role_count = 0u;
    std::uint32_t window_count = 0u;
    std::uint32_t cleanup_count = 0u;
    std::uint32_t inner_subscriptions = 0u;
    std::uint32_t fail_role = 99u;

    explicit InitRuntime(std::vector<std::string>& log) : order(log) {}

    std::uint32_t FindImage(std::string_view,
                            std::string_view) override {
        return 0x7000u + role_count;
    }
    void InitializeNameBuffer() override {
        ++role_count;
        window_count = 0u;
        order.push_back("init:" + std::to_string(role_count - 1u));
    }
    std::string FormatName(std::string_view pattern,
                           std::int32_t index) override {
        return std::string(pattern) + ":" + std::to_string(index);
    }
    std::uint32_t FindWindow(std::string_view) override {
        if (role_count - 1u == fail_role)
            throw std::runtime_error("find window");
        return 0x5000u + (role_count - 1u) * 0x200u +
               window_count++ * 0x10u;
    }
    void SetRoleIndex(std::uint32_t, std::uint32_t) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    void SetFont(std::uint32_t, std::string_view) override {}
    void SetTextFormatting(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    std::uint32_t WidthBits(std::uint32_t) override { return 0x3f800000u; }
    void Subscribe(std::uint32_t, pal4::RoleMouseEvent event,
                   std::uint32_t handler, std::uint32_t owner,
                   bool) override {
        Check(event != pal4::RoleMouseEvent::ButtonUp &&
                  (handler == 0x476910u || handler == 0x476970u) &&
                  owner == 0x3000u,
              "member hover subscriptions retain owner and handlers");
        ++inner_subscriptions;
    }
    std::uint32_t CleanupNameBuffer() override {
        ++cleanup_count;
        return 0u;
    }
};

struct ConstructorRuntime final : pal4::IRoleStateWindowConstructorRuntime {
    std::vector<std::string>& order;
    struct Binding {
        std::uint32_t window;
        pal4::RoleMouseEvent event;
        std::uint32_t callback;
    };
    std::vector<Binding> bindings;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> released;
    bool fail_subscription = false;
    bool null_callback = false;
    bool zero_connection = false;

    explicit ConstructorRuntime(std::vector<std::string>& log) : order(log) {}

    void LoadLayout(pal4::RoleStateWindowState& state,
                    std::uint32_t name) override {
        Check(name == 0x8b00e8u, "registered role window name");
        order.emplace_back("layout");
        state.Write(36u, 0x9000u);
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "24-byte callback allocation");
        return null_callback ? 0u :
               0xa000u + static_cast<std::uint32_t>(bindings.size()) * 0x20u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            pal4::RoleMouseEvent event,
                            std::uint32_t callback) override {
        bindings.push_back({window, event, callback});
        order.emplace_back("click-or-move");
        if (fail_subscription) throw std::runtime_error("subscribe");
        return zero_connection ? 0u :
               0xb000u + static_cast<std::uint32_t>(bindings.size() - 1u) *
                             0x20u;
    }
    void ReleaseConnection(std::uint32_t connection) override {
        released.push_back(connection);
    }
};

void TestFourMembersAndRoot() {
    std::vector<std::string> order;
    InitRuntime init(order);
    ConstructorRuntime runtime(order);
    pal4::RoleStateWindowState state;
    state.Write(592u, 0xffffffffu);
    Check(pal4::RoleStateWindowConstructor::Construct(
              state, 0x3000u, 0x8b00e8u, init, runtime) == 0x3000u,
          "constructor returns original object");
    Check(state.Read(0u) == 0x843ab0u &&
              state.Read(40u) == 0x843a98u && state.Read(592u) == 0u,
          "derived vtables and trailing member initialized");
    Check(init.role_count == 4u && init.cleanup_count == 4u &&
              init.inner_subscriptions == 16u &&
              runtime.bindings.size() == 9u &&
              runtime.released.size() == 9u,
          "four members, hover bindings and nine constructor bindings");
    Check(order.size() == 14u && order.front() == "layout" &&
              order.back() == "click-or-move",
          "layout precedes member initialization and root binding is last");
    for (std::uint32_t role = 0u; role < 4u; ++role) {
        Check(order[1u + role * 3u] ==
                  "init:" + std::to_string(role) &&
                  order[2u + role * 3u] == "click-or-move" &&
                  order[3u + role * 3u] == "click-or-move",
              "each member initialized before its two click bindings");
        const auto base = 44u + role * 0x88u;
        Check(state.Read(base + 8u) == role &&
                  state.Read(base + 116u) == 0x3000u &&
                  state.Read(base + 12u) == 0x5000u + role * 0x200u &&
                  state.Read(base + 96u) == 0x5130u + role * 0x200u,
              "member stride, role index, owner and control offsets");
        for (std::uint32_t part = 0u; part < 2u; ++part) {
            const auto& binding = runtime.bindings[role * 2u + part];
            Check(binding.window ==
                      0x5000u + role * 0x200u + part * 0x130u &&
                      binding.event == pal4::RoleMouseEvent::ButtonUp &&
                      binding.callback ==
                          0xa000u + (role * 2u + part) * 0x20u &&
                      runtime.words.at(binding.callback) == 0x843a90u &&
                      runtime.words.at(binding.callback + 8u) == 0x476fc0u &&
                      runtime.words.at(binding.callback + 12u) == 0u &&
                      runtime.words.at(binding.callback + 16u) == 0x3000u &&
                      runtime.released[role * 2u + part] ==
                          0xb000u + (role * 2u + part) * 0x20u,
                  "button-up target, handler and owner");
        }
    }
    const auto& root = runtime.bindings.back();
    Check(root.window == 0x9000u &&
              root.event == pal4::RoleMouseEvent::Move &&
              root.callback == 0xa100u &&
              runtime.words.at(root.callback + 8u) == 0x4773f0u &&
              runtime.words.at(root.callback + 16u) == 0x3000u &&
              runtime.released.back() == 0xb100u,
          "root mouse-move subscription");
}

void TestFailureStopsConstruction() {
    std::vector<std::string> order;
    InitRuntime init(order);
    ConstructorRuntime runtime(order);
    pal4::RoleStateWindowState state;
    init.fail_role = 2u;
    try {
        static_cast<void>(pal4::RoleStateWindowConstructor::Construct(
            state, 0x3000u, 0x8b00e8u, init, runtime));
        Check(false, "window lookup failure propagates");
    } catch (const std::runtime_error&) {
        Check(init.cleanup_count == 3u && runtime.bindings.size() == 4u &&
                  state.Read(44u + 2u * 0x88u) == 0u,
              "failed member cleans text and stops later bindings");
    }

    order.clear();
    InitRuntime init_again(order);
    ConstructorRuntime failing_runtime(order);
    failing_runtime.fail_subscription = true;
    pal4::RoleStateWindowState second;
    try {
        static_cast<void>(pal4::RoleStateWindowConstructor::Construct(
            second, 0x3000u, 0x8b00e8u, init_again,
            failing_runtime));
        Check(false, "subscription failure propagates");
    } catch (const std::runtime_error&) {
        Check(init_again.cleanup_count == 1u &&
                  failing_runtime.bindings.size() == 1u &&
                  failing_runtime.released.empty(),
              "subscription failure prevents later member initialization");
    }

    order.clear();
    InitRuntime null_init(order);
    ConstructorRuntime null_runtime(order);
    null_runtime.null_callback = true;
    null_runtime.zero_connection = true;
    pal4::RoleStateWindowState null_state;
    Check(pal4::RoleStateWindowConstructor::Construct(
              null_state, 0x3000u, 0x8b00e8u,
              null_init, null_runtime) == 0x3000u &&
              null_runtime.bindings.size() == 9u &&
              null_runtime.words.empty() &&
              null_runtime.released.empty(),
          "allocation failure forwards null callback and null connection skips release");
    for (const auto& binding : null_runtime.bindings)
        Check(binding.callback == 0u, "null callback forwarded to every subscription");
}

}  // namespace

int main() {
    TestFourMembersAndRoot();
    TestFailureStopsConstruction();
}
