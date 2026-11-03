#include "pal4/introduction_window_elements.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Subscription {
    std::uint32_t window{};
    std::uint32_t callback{};
    std::uint32_t user_id{};
};

struct Runtime final : pal4::IntroductionWindowElementsRuntimeStub {
    std::vector<std::string> lookups;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> handlers;
    std::vector<std::uint32_t> manual_handlers;
    std::vector<std::uint32_t> mouse_windows;
    std::unordered_map<std::uint32_t, std::uint32_t> user_ids;
    std::uint32_t callback_count{};
    std::uint32_t constructed_count{};
    std::uint32_t manual_count{};
    std::uint32_t deleted_count{};
    std::uint32_t freed_callback{};
    std::uint32_t temporary_buffer = 0x7000u;
    std::uint8_t prefix_byte = 0u;
    std::uint8_t written_prefix = 0xffu;
    std::uint8_t initial_byte{};
    std::uint32_t freed_temporary{};
    std::uint32_t reference_after = 0xffffffffu;
    std::uint32_t initial_reference = 1u;
    bool final_append_succeeds = true;
    bool throw_callback_constructor{};
    bool allocation_fails{};
    std::size_t fail_find_at = 0u;
    std::vector<std::size_t> final_lengths;

    void InitializeTemporary(std::uint8_t byte,
                             std::uint8_t error_flag) override {
        Check(error_flag == 0u, "temporary name buffer clears error flag");
        initial_byte = byte;
    }
    std::string_view LayoutPrefix(
        const pal4::IntroductionWindowState&) override {
        return "intro/";
    }
    bool AllocateFinalName(std::size_t length,
                           std::uint8_t mode) override {
        Check(mode == 0u, "final name append uses allocation mode zero");
        final_lengths.push_back(length);
        return final_append_succeeds;
    }
    std::uint32_t FindWindow(std::string_view name) override {
        lookups.emplace_back(name);
        if (fail_find_at == lookups.size()) {
            throw std::runtime_error("find-window");
        }
        return 0x1000u + static_cast<std::uint32_t>(lookups.size() * 0x100u);
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "all click callbacks allocate 24 bytes");
        ++callback_count;
        return allocation_fails ? 0u : 0x4000u + callback_count * 0x20u;
    }
    std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t zero, std::uint32_t owner) override {
        Check(storage >= 0x4020u && zero == 0u && owner == 0x8000u,
              "callback constructor receives storage, zero and owner");
        ++constructed_count;
        handlers.push_back(handler);
        if (throw_callback_constructor) {
            throw std::runtime_error("callback-constructor");
        }
        return storage;
    }
    void InitializeManualCallback(
        std::uint32_t storage, std::uint32_t vtable,
        std::uint32_t handler, std::uint32_t zero,
        std::uint32_t owner) override {
        Check(storage >= 0x4020u && vtable == 0x84356cu &&
                  zero == 0u && owner == 0x8000u,
              "manual callback uses observed vtable and owner");
        ++manual_count;
        manual_handlers.push_back(handler);
    }
    void FreeCallbackStorage(std::uint32_t storage) override {
        freed_callback = storage;
    }
    void SetWindowUserId(std::uint32_t window,
                         std::uint32_t id) override {
        user_ids[window] = id;
    }
    std::uint32_t SubscribeClicked(
        std::uint32_t window, std::uint32_t callback) override {
        const auto found = user_ids.find(window);
        subscriptions.push_back({
            window, callback,
            found == user_ids.end() ? 0xffffffffu : found->second});
        return callback == 0u ? 0u : callback + 0x10000u;
    }
    std::uint32_t ReferenceCount(std::uint32_t reference) override {
        Check(reference >= 0x14020u,
              "subscription reference points to allocated callback");
        return initial_reference;
    }
    void SetReferenceCount(std::uint32_t,
                           std::uint32_t count) override {
        reference_after = count;
    }
    void DeleteCallback(std::uint32_t,
                        std::uint32_t flags) override {
        Check(flags == 1u, "zero reference deletes callback with flag one");
        ++deleted_count;
    }
    void SetupMouseHandlers(std::uint32_t member,
                            std::uint32_t window) override {
        Check(member == 0x8028u,
              "mouse setup uses IntroductionWindow member +0x28");
        mouse_windows.push_back(window);
    }
    std::uint32_t TemporaryBufferAddress() override {
        return temporary_buffer;
    }
    std::uint8_t ReadPrefixByte(std::uint32_t address) override {
        Check(address == 0x6fffu,
              "temporary buffer reference byte precedes data");
        return prefix_byte;
    }
    void WritePrefixByte(std::uint32_t address,
                         std::uint8_t value) override {
        Check(address == 0x6fffu,
              "reference decrement writes preceding byte");
        written_prefix = value;
    }
    std::uint32_t FreeTemporary(std::uint32_t address) override {
        Check(address == 0x6fffu,
              "temporary string frees allocation prefix");
        freed_temporary = address;
        return 0xdeadbeefu;
    }
};

const std::array<std::string_view, 12> kNames{
    "intro/BtnClose", "intro/BtnHistory", "intro/BtnStory",
    "intro/BtnWorldview", "intro/BtnSword", "intro/BtnSub1",
    "intro/BtnSub2", "intro/BtnSub3", "intro/BtnSub4",
    "intro/BtnSub5", "intro/EditInfo", "intro/PanelInfo"};

void CheckLookupAndSubscriptions() {
    pal4::IntroductionWindowState state{};
    Runtime runtime;
    Check(pal4::IntroductionWindowElements::Initialize(
              state, 0x8000u, 0x5au, runtime) == 0xdeadbeefu &&
              runtime.initial_byte == 0x5au &&
              runtime.lookups.size() == kNames.size() &&
              runtime.subscriptions.size() == 12u &&
              runtime.callback_count == 12u &&
              runtime.constructed_count == 6u &&
              runtime.manual_count == 6u &&
              runtime.deleted_count == 12u &&
              runtime.final_lengths == std::vector<std::size_t>{
                  14u, 15u},
          "all children and twelve subscriptions follow observed path");
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        Check(runtime.lookups[i] == kNames[i],
              "child lookup preserves name spelling and order");
    }
    for (std::size_t i = 0; i < 10u; ++i) {
        const auto offset = i == 0u ? 0x84u : 0x84u +
                           static_cast<std::uint32_t>(i * 4u);
        Check(state.Read(offset) == 0x1100u + i * 0x100u,
              "button windows occupy consecutive fields +0x84..+0xa8");
    }
    Check(state.Read(0x80u) == 0x1b00u &&
              state.Read(0xacu) == 0x1c00u &&
              state.Read(0xb4u) == 0x8000u,
          "EditInfo, PanelInfo and self fields are stored");
    Check(runtime.subscriptions[2].user_id == 0xffffffffu &&
              runtime.subscriptions[3].user_id == 1u &&
              runtime.subscriptions[4].user_id == 0xffffffffu &&
              runtime.subscriptions[5].user_id == 2u,
          "Story and Worldview gain IDs between duplicate subscriptions");
    Check(runtime.subscriptions[1].user_id == 0u &&
              runtime.subscriptions[6].user_id == 3u &&
              runtime.subscriptions[7].user_id == 0xffffffffu,
          "History/Sword IDs precede subscriptions while sub IDs follow");
    Check(runtime.handlers == std::vector<std::uint32_t>{
              0x44a150u, 0x447120u, 0x447120u,
              0x447120u, 0x447120u, 0x447120u} &&
              runtime.manual_handlers == std::vector<std::uint32_t>{
                  0x447120u, 0x447140u, 0x447140u,
                  0x447140u, 0x447140u, 0x447140u},
          "close, main and sub callbacks use the observed handler addresses");
    Check(runtime.mouse_windows == std::vector<std::uint32_t>{
              0x1200u, 0x1300u, 0x1400u, 0x1500u},
          "mouse handlers attach to four main buttons");
}

void CheckReferenceAndFailurePaths() {
    pal4::IntroductionWindowState state{};
    Runtime shared;
    shared.initial_reference = 2u;
    shared.prefix_byte = 4u;
    Check(pal4::IntroductionWindowElements::Initialize(
              state, 0x8000u, 0u, shared) == 0x6fffu &&
              shared.reference_after == 1u &&
              shared.deleted_count == 0u &&
              shared.written_prefix == 3u &&
              shared.freed_temporary == 0u,
          "shared callbacks and temporary string decrement without free");

    Runtime saturated;
    saturated.prefix_byte = 0xffu;
    (void)pal4::IntroductionWindowElements::Initialize(
        state, 0x8000u, 0u, saturated);
    Check(saturated.freed_temporary == 0x6fffu,
          "saturated temporary prefix is freed");

    Runtime missing_append;
    missing_append.final_append_succeeds = false;
    (void)pal4::IntroductionWindowElements::Initialize(
        state, 0x8000u, 0u, missing_append);
    Check(missing_append.lookups[10] == "intro/" &&
              missing_append.lookups[11] == "intro/",
          "final-name allocation failure looks up unchanged prefix");

    Runtime zero_allocation;
    zero_allocation.allocation_fails = true;
    zero_allocation.temporary_buffer = 0u;
    Check(pal4::IntroductionWindowElements::Initialize(
              state, 0x8000u, 0u, zero_allocation) == 0u &&
              zero_allocation.callback_count == 12u &&
              zero_allocation.constructed_count == 0u &&
              zero_allocation.manual_count == 0u &&
              zero_allocation.deleted_count == 0u &&
              zero_allocation.freed_temporary == 0u,
          "null callback allocations and null temporary buffer skip cleanup");

    Runtime throw_construct;
    throw_construct.throw_callback_constructor = true;
    try {
        (void)pal4::IntroductionWindowElements::Initialize(
            state, 0x8000u, 0u, throw_construct);
        Check(false, "callback constructor exception must propagate");
    } catch (const std::runtime_error&) {
        Check(throw_construct.freed_callback == 0x4020u &&
                  throw_construct.freed_temporary == 0x6fffu,
              "callback construction failure frees storage and temp string");
    }

    Runtime throw_find;
    throw_find.fail_find_at = 4u;
    try {
        (void)pal4::IntroductionWindowElements::Initialize(
            state, 0x8000u, 0u, throw_find);
        Check(false, "window lookup exception must propagate");
    } catch (const std::runtime_error&) {
        Check(throw_find.freed_temporary == 0x6fffu,
              "lookup exception releases temporary name buffer");
    }
}

void CheckConstructorIntegration() {
    struct LifecycleRuntime final :
        pal4::IntroductionWindowLifecycleRuntimeStub {
        Runtime elements;
        void InitializePanelElements(
            pal4::IntroductionWindowState& state) override {
            (void)pal4::IntroductionWindowElements::Initialize(
                state, 0x8000u, 0u, elements);
        }
    } runtime;
    pal4::IntroductionWindowState state{};
    (void)pal4::IntroductionWindowLifecycle::Construct(
        state, 0x8000u, 0x5000u, runtime);
    Check(runtime.elements.lookups.size() == 12u &&
              state.Read(0xb4u) == 0x8000u,
          "IntroductionWindow constructor reaches panel element initialization");
}

}  // namespace

int main() {
    CheckLookupAndSubscriptions();
    CheckReferenceAndFailurePaths();
    CheckConstructorIntegration();
    std::cout << "Introduction window elements tests passed\n";
}
