#include "pal4/shop_ui_frame.h"

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiFrameRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint8_t> memory;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> vectors;
    std::vector<std::uint32_t> freed;
    bool fail_mouse = false;
    bool fail_controls = false;
    std::uint32_t fail_vector = 0u;

    std::uint8_t ReadByte(const std::uint32_t address) override {
        const auto it = memory.find(address);
        return it == memory.end() ? 0u : it->second;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        memory[address] = value;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        std::uint32_t result = 0u;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            result |= static_cast<std::uint32_t>(ReadByte(address + i)) <<
                      (i * 8u);
        }
        return result;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            WriteByte(address + i,
                      static_cast<std::uint8_t>(value >> (i * 8u)));
        }
    }
    void LoadLayout(const std::uint32_t frame,
                    const std::uint32_t name,
                    const std::uint32_t left,
                    const std::uint32_t top) override {
        Check(frame == kFrame && name == 0x9010u &&
                  left == 0u && top == 0u,
              "shop layout arguments");
        calls.emplace_back("layout");
    }
    void ConstructMouse(const std::uint32_t mouse) override {
        Check(mouse == kFrame + 0x28u, "embedded mouse offset");
        calls.emplace_back("mouse");
        if (fail_mouse) throw std::runtime_error("mouse");
    }
    void ConstructVector2(const std::uint32_t vector) override {
        Check(vector == kFrame + 0x80u + vectors.size() * 8u,
              "vector positions are contiguous with 8-byte stride");
        calls.emplace_back("vector");
        vectors.push_back(vector);
        if (vectors.size() == fail_vector) {
            throw std::runtime_error("vector");
        }
    }
    void InitializeControls(const std::uint32_t frame) override {
        Check(frame == kFrame, "shop control receiver");
        calls.emplace_back("controls");
        if (fail_controls) throw std::runtime_error("controls");
    }
    void CleanupBase(const std::uint32_t frame) override {
        Check(frame == kFrame, "exception base cleanup receiver");
        calls.emplace_back("base cleanup");
    }
    void CleanupMouse(const std::uint32_t mouse) override {
        Check(mouse == kFrame + 0x28u, "exception mouse cleanup receiver");
        calls.emplace_back("mouse cleanup");
    }
    void CleanupText(const std::uint32_t text) override {
        Check(text == kFrame + 0x60u || text == kFrame + 0x70u,
              "exception text cleanup receiver");
        calls.emplace_back(text == kFrame + 0x70u ?
                           "second text cleanup" : "first text cleanup");
    }
    void Free(const std::uint32_t address) override {
        calls.emplace_back("free");
        freed.push_back(address);
    }
    std::uint32_t DestroyBase(const std::uint32_t frame) override {
        Check(frame == kFrame, "ordinary base destructor receiver");
        calls.emplace_back("base destroy");
        return 0xbeefu;
    }
    std::uint32_t CallVirtual(const std::uint32_t function,
                              const std::uint32_t frame) override {
        Check(function == 0x1234u && frame == kFrame,
              "virtual slot three target and receiver");
        calls.emplace_back("virtual");
        return 0x7788u;
    }
};

void CheckCleanupTail(const Runtime& runtime,
                      const std::vector<std::string>& expected) {
    Check(runtime.calls.size() >= expected.size(), "cleanup tail length");
    Check(std::vector<std::string>(
              runtime.calls.end() - static_cast<std::ptrdiff_t>(
                  expected.size()), runtime.calls.end()) == expected,
          "exception cleanup order");
}

void TestConstructionAndVirtualForwarding() {
    Runtime runtime;
    Check(pal4::ShopUiFrame::Construct(
              Runtime::kFrame, 0x9010u, runtime) == Runtime::kFrame &&
              runtime.vectors.size() == 8u &&
              runtime.vectors.back() == Runtime::kFrame + 0xb8u &&
              runtime.ReadWord(Runtime::kFrame) == 0x843d04u &&
              runtime.ReadWord(Runtime::kFrame + 0x28u) == 0x843cdcu &&
              runtime.ReadByte(Runtime::kFrame + 0x60u) == 0x10u &&
              runtime.ReadByte(Runtime::kFrame + 0x70u) == 0x10u &&
              runtime.ReadWord(Runtime::kFrame + 0x64u) == 0u &&
              runtime.ReadWord(Runtime::kFrame + 0x74u) == 0u &&
              runtime.calls.front() == "layout" &&
              runtime.calls[1] == "mouse" &&
              runtime.calls.back() == "controls",
          "outer shop constructor initializes layout, members and controls");

    runtime.WriteWord(Runtime::kFrame, 0xa000u);
    runtime.WriteWord(0xa00cu, 0x1234u);
    Check(pal4::ShopUiFrame::ForwardVirtualSlotThree(
              Runtime::kFrame, runtime) == 0x7788u &&
              runtime.calls.back() == "virtual",
          "virtual forwarding reads vtable slot +0xc");
}

void TestConstructorUnwind() {
    Runtime mouse;
    mouse.fail_mouse = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ShopUiFrame::Construct(
            Runtime::kFrame, 0x9010u, mouse));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught, "mouse construction throws");
    CheckCleanupTail(mouse, {"mouse", "base cleanup"});

    Runtime vector;
    vector.fail_vector = 3u;
    caught = false;
    try {
        static_cast<void>(pal4::ShopUiFrame::Construct(
            Runtime::kFrame, 0x9010u, vector));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && vector.vectors.size() == 3u,
          "third vector constructor throws after first two");
    CheckCleanupTail(vector, {"second text cleanup", "first text cleanup",
                              "mouse cleanup", "base cleanup"});

    Runtime controls;
    controls.fail_controls = true;
    caught = false;
    try {
        static_cast<void>(pal4::ShopUiFrame::Construct(
            Runtime::kFrame, 0x9010u, controls));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && controls.vectors.size() == 8u,
          "control initializer throws after vectors");
    CheckCleanupTail(controls, {"second text cleanup", "first text cleanup",
                                "mouse cleanup", "base cleanup"});
}

void TestDestruction() {
    Runtime runtime;
    runtime.WriteWord(Runtime::kFrame + 0x74u, 0x8001u);
    runtime.WriteWord(Runtime::kFrame + 0x78u, 5u);
    runtime.WriteWord(Runtime::kFrame + 0x7cu, 8u);
    runtime.WriteByte(0x8000u, 0xffu);
    runtime.WriteWord(Runtime::kFrame + 0x64u, 0x7001u);
    runtime.WriteWord(Runtime::kFrame + 0x68u, 5u);
    runtime.WriteWord(Runtime::kFrame + 0x6cu, 8u);
    runtime.WriteByte(0x7000u, 2u);
    runtime.WriteWord(Runtime::kFrame + 0x30u, 0x9000u);
    runtime.WriteWord(Runtime::kFrame + 0x34u, 1u);
    runtime.WriteWord(Runtime::kFrame + 0x38u, 2u);
    Check(pal4::ShopUiFrame::Destroy(Runtime::kFrame, runtime) ==
              0xbeefu &&
              runtime.freed ==
                  std::vector<std::uint32_t>{0x8000u, 0x9000u} &&
              runtime.ReadByte(0x7000u) == 1u &&
              runtime.ReadWord(Runtime::kFrame + 0x74u) == 0u &&
              runtime.ReadWord(Runtime::kFrame + 0x7cu) == 0u &&
              runtime.ReadWord(Runtime::kFrame + 0x64u) == 0u &&
              runtime.ReadWord(Runtime::kFrame + 0x6cu) == 0u &&
              runtime.ReadWord(Runtime::kFrame + 0x30u) == 0u &&
              runtime.ReadWord(Runtime::kFrame + 0x38u) == 0u &&
              runtime.calls.back() == "base destroy",
          "destructor releases text in reverse order then mouse and base");

    Runtime zero_reference;
    zero_reference.WriteWord(Runtime::kFrame + 0x74u, 0x8001u);
    zero_reference.WriteByte(0x8000u, 0u);
    Check(pal4::ShopUiFrame::Destroy(
              Runtime::kFrame, zero_reference) == 0xbeefu &&
              zero_reference.freed ==
                  std::vector<std::uint32_t>{0x8000u, 0u},
          "zero reference frees text and null mouse pointer is passed to free");
}

}  // namespace

int main() {
    TestConstructionAndVirtualForwarding();
    TestConstructorUnwind();
    TestDestruction();
    return EXIT_SUCCESS;
}
