#include "pal4/pal_font_manager.h"

#include <cstdlib>
#include <iostream>
#include <map>
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

class AccessRuntime final : public pal4::IPalFontManagerAccessRuntime {
public:
    std::uint32_t GetSingletonManager() override {
        ++manager_calls;
        return 0x6000u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destroy) override {
        ++register_calls;
        Check(manager == 0x6000u && name == "palFontManagerA" &&
                  id == 1004u && factory == 0x419f80u &&
                  destroy == 0x419fe0u,
              "singleton registration arguments match IDA");
        return registration_result;
    }
    std::uint32_t registration_result{0x7000u};
    int manager_calls{};
    int register_calls{};
};

class ObjectRuntime final : public pal4::IPalFontManagerObjectRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate");
        Check(bytes == 28u, "factory allocates 28 bytes");
        return allocation_result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return free_result;
    }
    pal4::PalFontManagerState& Access(std::uint32_t address) override {
        Check(address == 0x1000u, "object access uses allocation address");
        if (throw_on_access) {
            throw std::runtime_error("construct");
        }
        return state;
    }
    void CallEmptyDestructor(std::uint32_t address) override {
        events.push_back("destruct:" + std::to_string(address));
    }

    pal4::PalFontManagerState state;
    std::uint32_t allocation_result{0x1000u};
    std::uint32_t free_result{0x4321u};
    bool throw_on_access{};
    std::vector<std::string> events;
};

class FontRuntime final : public pal4::IPalFontManagerFontRuntime {
public:
    std::uint32_t Log(std::uint32_t source_line) override {
        events.push_back("log:" + std::to_string(source_line));
        return source_line + 0x100u;
    }
    void CreateFont(std::uint32_t slot,
                    const pal4::PalFontDescription& description) override {
        events.push_back("font:" + std::to_string(slot));
        weights.push_back(description.weight);
        fourths.push_back(description.fourth);
        Check(description.height == 0 && description.width == 0 &&
                  description.sixth == 0 &&
                  description.face_name == 0x8a6068u &&
                  description.options ==
                      std::array<std::uint8_t, 4>{1u, 0u, 0u, 0u},
              "font description retains zero fields, Times New Roman and options");
    }
    void InsertOne(pal4::PalFontManagerState& state,
                   std::uint32_t slot) override {
        events.push_back("insert:" + std::to_string(slot));
        const auto end = state.ReadDword(8u);
        words[end] = slot;
        state.WriteDword(8u, end + 4u);
    }
    std::uint32_t AllocateArray(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return allocation_result;
    }
    std::uint32_t ReadArray(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteArray(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void FreeArray(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }

    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::uint32_t> weights;
    std::vector<std::uint32_t> fourths;
    std::uint32_t allocation_result{0x4000u};
};

class RenderRuntime final : public pal4::IRenderStateObjectRuntime {
public:
    std::uint32_t CallSlot(std::uint32_t object,
                           std::uint32_t byte_offset) override {
        calls.push_back({object, byte_offset});
        return 0x7777u;
    }
    std::uint32_t DrawRectangleSlot(
        std::uint32_t object, std::uint32_t first,
        std::int32_t command, std::int32_t sentinel,
        const std::array<std::int32_t, 4>& rectangle,
        std::int32_t last, std::int32_t penultimate) override {
        Check(object == 0x2222u && first == 0u &&
                  command == 17 && sentinel == -1 &&
                  rectangle == std::array<std::int32_t, 4>{1, 2, 3, 4} &&
                  last == 99 && penultimate == 88,
              "indexed rectangle preserves wrapper argument order");
        ++draw_calls;
        return 0x8888u;
    }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
    int draw_calls{};
};

void SingletonCachesFirstRegistration() {
    AccessRuntime runtime;
    pal4::PalFontManagerAccessState state;
    Check(pal4::PalFontManagerAccess::Get(state, runtime) == 0x7000u &&
              state.instance == 0x7000u && state.guard == 1u,
          "cold singleton access registers and caches instance");
    runtime.registration_result = 0x8000u;
    Check(pal4::PalFontManagerAccess::Get(state, runtime) == 0x7000u &&
              runtime.manager_calls == 1 && runtime.register_calls == 1,
          "warm singleton access returns cached instance");

    pal4::PalFontManagerAccessState failed{0x80u, 0x9999u};
    runtime.registration_result = 0u;
    Check(pal4::PalFontManagerAccess::Get(failed, runtime) == 0u &&
              failed.guard == 0x81u && failed.instance == 0u,
          "failed registration retains guard and caches null");
    Check(pal4::PalFontManagerAccess::Get(failed, runtime) == 0u &&
              runtime.register_calls == 2,
          "failed registration is not retried");
}

void FactoryAndConstructorPreserveUntouchedBytes() {
    ObjectRuntime runtime;
    runtime.state.Bytes().fill(0xa5u);
    Check(pal4::PalFontManagerObject::Create(0x6bu, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "factory allocates and returns constructed address");
    Check(runtime.state.Bytes()[0] == 0x6bu,
          "constructor writes incoming stack byte");
    for (std::uint32_t offset = 1u; offset < 28u; ++offset) {
        const auto expected = offset >= 4u && offset < 16u ? 0u : 0xa5u;
        Check(runtime.state.Bytes()[offset] == expected,
              "constructor changes only observed bytes");
    }
    Check(pal4::PalFontManagerObject::Construct(0x1000u, 0x3cu, runtime) ==
              0x1000u && runtime.state.Bytes()[0] == 0x3cu,
          "direct constructor returns this");
}

void FactoryFailurePaths() {
    ObjectRuntime runtime;
    runtime.allocation_result = 0u;
    Check(pal4::PalFontManagerObject::Create(0u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "allocation failure returns null without construction");

    runtime.allocation_result = 0x1000u;
    runtime.throw_on_access = true;
    runtime.events.clear();
    try {
        static_cast<void>(pal4::PalFontManagerObject::Create(0u, runtime));
        Check(false, "construction failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>({
                  "allocate", "free:4096"}),
              "factory frees allocation after construction failure");
    }
}

void TidyAndDestroyOrder() {
    ObjectRuntime runtime;
    runtime.state.Bytes().fill(0xa5u);
    runtime.state.WriteDword(4u, 0x5000u);
    runtime.state.WriteDword(8u, 0x6000u);
    runtime.state.WriteDword(12u, 0x7000u);
    Check(pal4::PalFontManagerObject::Tidy(0x1000u, runtime) == 0x4321u &&
              runtime.events == std::vector<std::string>({
                  "destruct:4120", "destruct:4116", "destruct:4112",
                  "free:20480"}),
          "tidy calls three empty destructors in reverse order and frees +4");
    Check(runtime.state.ReadDword(4u) == 0u &&
              runtime.state.ReadDword(8u) == 0u &&
              runtime.state.ReadDword(12u) == 0u &&
              runtime.state.Bytes()[0] == 0xa5u &&
              runtime.state.Bytes()[16] == 0xa5u,
          "tidy clears pointer triplet and preserves other bytes");

    runtime.events.clear();
    pal4::PalFontManagerObject::Destroy(0u, runtime);
    Check(runtime.events.empty(), "null destroy makes no calls");
    pal4::PalFontManagerObject::Destroy(0x1000u, runtime);
    Check(runtime.events == std::vector<std::string>({
              "destruct:4120", "destruct:4116", "destruct:4112",
              "free:0", "free:4096"}),
          "destroy tidies before releasing the object");

    pal4::PalFontManagerObjectRuntimeStub stub;
    const auto address = pal4::PalFontManagerObject::Create(0u, stub);
    Check(address != 0u, "linkable font-manager runtime allocates an object");
    pal4::PalFontManagerObject::Destroy(address, stub);
}

void InitializeFontsGrowsFullArray() {
    pal4::PalFontManagerState state;
    state.WriteDword(4u, 0x3000u);
    state.WriteDword(8u, 0x3000u);
    state.WriteDword(12u, 0x3008u);
    FontRuntime runtime;
    Check(pal4::PalFontManagerFonts::Initialize(
              0x1000u, state, runtime) == 0x122u,
          "initialization returns final logger result");
    Check(runtime.weights == std::vector<std::uint32_t>({
              400u, 800u, 400u}) &&
              runtime.fourths == std::vector<std::uint32_t>({
                  0u, 0u, 1u}),
          "three hardcoded font descriptions are ordered");
    Check(runtime.events == std::vector<std::string>({
              "log:21", "font:4112", "insert:4112",
              "font:4116", "insert:4116", "font:4120",
              "allocate:16", "write:16384", "write:16388",
              "write:16392", "free:12288", "log:34"}),
          "full array copies prior entries, appends third and frees old storage");
    Check(state.ReadDword(4u) == 0x4000u &&
              state.ReadDword(8u) == 0x400cu &&
              state.ReadDword(12u) == 0x4010u &&
              runtime.words.at(0x4000u) == 0x1010u &&
              runtime.words.at(0x4004u) == 0x1014u &&
              runtime.words.at(0x4008u) == 0x1018u,
          "growth sets begin/end/capacity and copies all three font slots");
}

void InitializeFontsUsesSpareCapacity() {
    pal4::PalFontManagerState state;
    state.WriteDword(4u, 0x3000u);
    state.WriteDword(8u, 0x3000u);
    state.WriteDword(12u, 0x300cu);
    FontRuntime runtime;
    static_cast<void>(pal4::PalFontManagerFonts::Initialize(
        0x1000u, state, runtime));
    Check(state.ReadDword(4u) == 0x3000u &&
              state.ReadDword(8u) == 0x300cu &&
              state.ReadDword(12u) == 0x300cu &&
              runtime.words.at(0x3008u) == 0x1018u,
          "third font appends in existing capacity");
    Check(runtime.events == std::vector<std::string>({
              "log:21", "font:4112", "insert:4112",
              "font:4116", "insert:4116", "font:4120",
              "write:12296", "log:34"}),
          "spare capacity avoids allocation and free");
}

void InitializeFontsAllocationFailureAndStub() {
    pal4::PalFontManagerState state;
    state.WriteDword(4u, 0x3000u);
    state.WriteDword(8u, 0x3000u);
    state.WriteDword(12u, 0x3008u);
    FontRuntime runtime;
    runtime.allocation_result = 0u;
    try {
        static_cast<void>(pal4::PalFontManagerFonts::Initialize(
            0x1000u, state, runtime));
        Check(false, "failed allocation must not access address zero");
    } catch (const std::bad_alloc&) {
        Check(state.ReadDword(4u) == 0x3000u &&
                  state.ReadDword(8u) == 0x3008u &&
                  state.ReadDword(12u) == 0x3008u,
              "failed growth preserves old array fields");
    }

    pal4::PalFontManagerState stub_state;
    pal4::PalFontManagerFontRuntimeStub stub;
    Check(pal4::PalFontManagerFonts::Initialize(
              0x1000u, stub_state, stub) == 0u &&
              stub_state.ReadDword(8u) - stub_state.ReadDword(4u) == 12u &&
              stub_state.ReadDword(12u) - stub_state.ReadDword(4u) == 16u,
          "linkable font runtime stores three pointer slots");
}

void ReleaseFontsClearsSlotsAndRewindsArray() {
    pal4::PalFontManagerState state;
    state.WriteDword(4u, 0x3000u);
    state.WriteDword(8u, 0x300cu);
    state.WriteDword(12u, 0x3010u);
    state.WriteDword(16u, 0x1111u);
    state.WriteDword(20u, 0u);
    state.WriteDword(24u, 0x3333u);
    RenderRuntime runtime;
    Check(pal4::PalFontManagerFonts::Release(state, runtime) == 0x300cu &&
              state.ReadDword(8u) == 0x3000u &&
              state.ReadDword(12u) == 0x3010u &&
              state.ReadDword(16u) == 0u &&
              state.ReadDword(20u) == 0u &&
              state.ReadDword(24u) == 0u,
          "release clears font slots and rewinds end without freeing storage");
    Check(runtime.calls ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                  {0x1111u, 8u}, {0x3333u, 8u}}),
          "release invokes non-null virtual slot +8 in order");
}

void TextureWalksUseSeparateSlots() {
    pal4::PalFontManagerState state;
    state.WriteDword(4u, 0x3000u);
    state.WriteDword(8u, 0x3008u);
    FontRuntime array_runtime;
    array_runtime.words[0x3000u] = 0x1111u;
    array_runtime.words[0x3004u] = 0x2222u;
    RenderRuntime render_runtime;
    Check(pal4::PalFontManagerFonts::GetTextures(
              state, array_runtime, render_runtime) == 0x3008u &&
              pal4::PalFontManagerFonts::GetTextureStates(
                  state, array_runtime, render_runtime) == 0x3008u,
          "texture passes return array end");
    Check(render_runtime.calls ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>({
                  {0x1111u, 0x40u}, {0x2222u, 0x40u},
                  {0x1111u, 0x44u}, {0x2222u, 0x44u}}),
          "texture passes visit every font with distinct virtual slots");
    state.WriteDword(8u, 0x3000u);
    render_runtime.calls.clear();
    Check(pal4::PalFontManagerFonts::GetTextures(
              state, array_runtime, render_runtime) == 0x3000u &&
              render_runtime.calls.empty(),
          "empty array makes no virtual calls");
}

void DrawRectangleSelectsFontByIndex() {
    pal4::PalFontManagerState state;
    state.WriteDword(4u, 0x3000u);
    FontRuntime array_runtime;
    array_runtime.words[0x3004u] = 0x2222u;
    RenderRuntime render_runtime;
    Check(pal4::PalFontManagerFonts::DrawRectangle(
              state, 17, {1, 2, 3, 4}, 1u, 88, 99,
              array_runtime, render_runtime) == 0x8888u &&
              render_runtime.draw_calls == 1,
          "draw selects second font and forwards result");
}

}  // namespace

int main() {
    SingletonCachesFirstRegistration();
    FactoryAndConstructorPreserveUntouchedBytes();
    FactoryFailurePaths();
    TidyAndDestroyOrder();
    InitializeFontsGrowsFullArray();
    InitializeFontsUsesSpareCapacity();
    InitializeFontsAllocationFailureAndStub();
    ReleaseFontsClearsSlotsAndRewindsArray();
    TextureWalksUseSeparateSlots();
    DrawRectangleSelectsFontByIndex();
    std::cout << "10 PAL font manager test cases passed\n";
}
