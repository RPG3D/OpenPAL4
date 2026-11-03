#include "pal4/ui_object_elements.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::UiObjectElementsRuntimeStub {
    std::vector<char> events;
    std::vector<std::string> names;
    std::vector<std::uint32_t> subscribed_callbacks;
    std::vector<std::uint32_t> deleted_callbacks;
    std::vector<std::uint32_t> mouse_windows;
    int prefix_reads{};
    int allocations{};
    bool website_allocation{true};
    bool zero_button_allocation{};
    bool fail_close_construction{};
    std::uint32_t freed_callback_storage{};
    std::uint32_t temporary_buffer{0xc000u};
    std::uint8_t prefix_byte{};
    std::uint8_t written_prefix{0xffu};
    std::uint32_t freed_temporary{};

    void InitializeTemporary(std::uint8_t byte,
                             std::uint8_t flag) override {
        Check(byte == 0x5au && flag == 0u,
              "temporary buffer receives explicit stack byte and zero flag");
        events.push_back('I');
    }
    std::string_view LayoutPrefix(const pal4::UiObjectState& state) override {
        Check(state.Read(0x14u) == 0x2300u,
              "each name starts from object +0x14");
        ++prefix_reads;
        events.push_back('P');
        return "Dialog/";
    }
    bool AllocateWebsiteName(std::size_t length,
                             std::uint8_t mode) override {
        Check(length == 17u && mode == 0u,
              "website append requests prefix plus suffix with mode zero");
        events.push_back('T');
        return website_allocation;
    }
    std::uint32_t FindWindow(std::string_view name) override {
        names.emplace_back(name);
        events.push_back('W');
        if (name == "Dialog/BtnClose") return 0x1100u;
        if (name == "Dialog/EditInfo") return 0x1200u;
        if (name == "Dialog/PanelInfo") return 0x1300u;
        if (name == "Dialog/BtnCast") return 0x1400u;
        if (name == "Dialog/BtnWebsite") return 0x1500u;
        if (name == "Dialog/") return 0x1600u;
        Check(false, "unexpected child window name");
        return 0u;
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "all three click callbacks allocate 0x18");
        ++allocations;
        events.push_back('A');
        if (zero_button_allocation && allocations == 2) return 0u;
        return static_cast<std::uint32_t>(0x4000u + allocations * 0x1000u);
    }
    std::uint32_t ConstructCloseCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t zero, std::uint32_t owner) override {
        Check(storage == 0x5000u &&
                  handler == pal4::UiObjectElements::kCloseHandler &&
                  zero == 0u && owner == 0x7000u,
              "close callback constructor receives handler and owner");
        events.push_back('C');
        if (fail_close_construction) throw std::runtime_error("close callback");
        return 0x5001u;
    }
    void FreeCallbackStorage(std::uint32_t storage) override {
        Check(storage == 0x5000u,
              "failed close constructor releases its allocation");
        freed_callback_storage = storage;
        events.push_back('X');
    }
    void InitializeButtonCallback(
        std::uint32_t storage, std::uint32_t vtable,
        std::uint32_t handler, std::uint32_t zero,
        std::uint32_t owner) override {
        Check((storage == 0x6000u || storage == 0x7000u) &&
                  vtable == pal4::UiObjectElements::kCallbackVtable &&
                  handler == pal4::UiObjectElements::kButtonHandler &&
                  zero == 0u && owner == 0x7000u,
              "Cast/Website callbacks have exact vtable and fields");
        events.push_back('B');
    }
    void SetWindowUserId(std::uint32_t window,
                         std::uint32_t id) override {
        Check((window == 0x1400u && id == 17u) ||
                  ((window == 0x1500u || window == 0x1600u) && id == 18u),
              "Cast and Website receive IDs 17 and 18");
        events.push_back('U');
    }
    std::uint32_t SubscribeClicked(std::uint32_t window,
                                   std::uint32_t callback) override {
        Check(window == 0x1100u || window == 0x1400u ||
                  window == 0x1500u || window == 0x1600u,
              "only three buttons subscribe to EventClicked");
        subscribed_callbacks.push_back(callback);
        events.push_back('S');
        if (window == 0x1100u) return 0x9000u;
        if (window == 0x1400u) return 0xa000u;
        return 0xb000u;
    }
    void ReleaseCloseReference(std::uint32_t reference) override {
        Check(reference == 0x9000u,
              "close uses CEGUI::Referenced::release");
        events.push_back('R');
    }
    std::uint32_t ReferenceCount(std::uint32_t reference) override {
        Check(reference == 0xa000u || reference == 0xb000u,
              "button callback reads its own reference word");
        events.push_back('Q');
        return reference == 0xa000u ? 1u : 2u;
    }
    void SetReferenceCount(std::uint32_t reference,
                           std::uint32_t count) override {
        Check((reference == 0xa000u && count == 0u) ||
                  (reference == 0xb000u && count == 1u),
              "button callback reference count decrements once");
        events.push_back('q');
    }
    void DeleteCallback(std::uint32_t reference,
                        std::uint32_t flags) override {
        Check(reference == 0xa000u && flags == 1u,
              "zero-reference callback uses deleting flag one");
        deleted_callbacks.push_back(reference);
        events.push_back('D');
    }
    void SetupMouseHandlers(pal4::UiObjectState&,
                            std::uint32_t offset,
                            std::uint32_t window) override {
        Check(offset == 0x28u,
              "mouse handlers are attached to member +0x28");
        mouse_windows.push_back(window);
        events.push_back('H');
    }
    std::uint32_t TemporaryBufferAddress() override {
        events.push_back('M');
        return temporary_buffer;
    }
    std::uint8_t ReadPrefixByte(std::uint32_t address) override {
        Check(address == 0xbfffu,
              "temporary reference byte precedes text data");
        events.push_back('r');
        return prefix_byte;
    }
    void WritePrefixByte(std::uint32_t address,
                         std::uint8_t value) override {
        Check(address == 0xbfffu,
              "temporary reference count writes preceding byte");
        written_prefix = value;
        events.push_back('w');
    }
    std::uint32_t FreeTemporary(std::uint32_t address) override {
        Check(address == 0xbfffu,
              "temporary storage free uses prefix address");
        freed_temporary = address;
        events.push_back('F');
        return 0x7777u;
    }
};

pal4::UiObjectState MakeState() {
    pal4::UiObjectState state{};
    state.Write(0x14u, 0x2300u);
    state.Write(0x54u, pal4::UiObjectState::kTailVtable);
    return state;
}

void CheckFullInitialization() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::UiObjectElements::Initialize(state, 0x7000u, 0x5au,
                                              runtime) == 0x7777u &&
              state.Read(0x40u) == 0x1200u &&
              state.Read(0x44u) == 0x1100u &&
              state.Read(0x48u) == 0x1400u &&
              state.Read(0x4cu) == 0x1500u &&
              state.Read(0x50u) == 0x1300u &&
              state.Read(0x58u) == 0x7000u &&
              state.Read(0x54u) == pal4::UiObjectState::kTailVtable,
          "five windows and object self pointer occupy observed fields");
    Check(runtime.prefix_reads == 5 &&
              runtime.names == std::vector<std::string>{
                  "Dialog/BtnClose", "Dialog/EditInfo", "Dialog/PanelInfo",
                  "Dialog/BtnCast", "Dialog/BtnWebsite"} &&
              runtime.subscribed_callbacks ==
                  std::vector<std::uint32_t>{0x5001u, 0x6000u, 0x7000u} &&
              runtime.deleted_callbacks ==
                  std::vector<std::uint32_t>{0xa000u} &&
              runtime.mouse_windows ==
                  std::vector<std::uint32_t>{0x1400u, 0x1500u} &&
              runtime.freed_temporary == 0xbfffu,
          "lookups, click callbacks, reference release and mouse hooks match IDA");
    Check(runtime.events == std::vector<char>{
              'I', 'P', 'W', 'A', 'C', 'S', 'R',
              'P', 'W', 'P', 'W',
              'P', 'W', 'U', 'A', 'B', 'S', 'Q', 'q', 'D',
              'P', 'T', 'W', 'U', 'A', 'B', 'S', 'Q', 'q',
              'H', 'H', 'M', 'r', 'F'},
          "window initialization preserves the observed operation order");
}

void CheckFallbacksAndTemporaryReferences() {
    auto state = MakeState();
    Runtime runtime;
    runtime.website_allocation = false;
    runtime.zero_button_allocation = true;
    runtime.prefix_byte = 3u;
    Check(pal4::UiObjectElements::Initialize(state, 0x7000u, 0x5au,
                                              runtime) == 0xbfffu &&
              state.Read(0x4cu) == 0x1600u &&
              runtime.names.back() == "Dialog/" &&
              runtime.subscribed_callbacks[1] == 0u &&
              runtime.written_prefix == 2u &&
              runtime.freed_temporary == 0u,
          "website allocation failure keeps prefix and nonzero temp ref decrements");

    runtime.events.clear();
    runtime.prefix_byte = 0xffu;
    runtime.zero_button_allocation = false;
    runtime.allocations = 0;
    Check(pal4::UiObjectElements::Initialize(state, 0x7000u, 0x5au,
                                              runtime) == 0x7777u &&
              runtime.freed_temporary == 0xbfffu,
          "0xff temporary reference frees its prefixed buffer");
    runtime.events.clear();
    runtime.temporary_buffer = 0u;
    runtime.allocations = 0;
    Check(pal4::UiObjectElements::Initialize(state, 0x7000u, 0x5au,
                                              runtime) == 0u &&
              runtime.events.back() == 'M',
          "null temporary buffer returns zero without reading prefix");
}

void CheckConstructionFailureCleanup() {
    auto state = MakeState();
    Runtime runtime;
    runtime.fail_close_construction = true;
    bool threw = false;
    try {
        (void)pal4::UiObjectElements::Initialize(state, 0x7000u,
                                                  0x5au, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.freed_callback_storage == 0x5000u &&
              runtime.freed_temporary == 0xbfffu &&
              runtime.events ==
                  std::vector<char>{'I', 'P', 'W', 'A', 'C', 'X', 'M', 'r', 'F'},
          "close callback failure frees its block and temporary text");
}

void CheckConstructorIntegration() {
    struct LifecycleRuntime final : pal4::UiObjectLifecycleRuntimeStub {
        Runtime& elements;
        explicit LifecycleRuntime(Runtime& source) : elements(source) {}
        void InitializeElements(pal4::UiObjectState& state) override {
            (void)pal4::UiObjectElements::Initialize(state, 0x7000u,
                                                      0x5au, elements);
        }
    };

    auto state = MakeState();
    Runtime elements;
    LifecycleRuntime lifecycle(elements);
    Check(&pal4::UiObjectLifecycle::Construct(state, 0x8800u, lifecycle) ==
              &state && state.Read(0u) == pal4::UiObjectState::kVtable &&
              state.Read(0x28u) ==
                  pal4::UiObjectState::kMemberVtable &&
              state.Read(0x44u) == 0x1100u &&
              state.Read(0x58u) == 0x7000u &&
              elements.names.size() == 5u,
          "UI object constructor can execute reconstructed element setup");
}

}  // namespace

int main() {
    CheckFullInitialization();
    CheckFallbacksAndTemporaryReferences();
    CheckConstructionFailureCleanup();
    CheckConstructorIntegration();
    std::cout << "UI object elements tests passed\n";
}
