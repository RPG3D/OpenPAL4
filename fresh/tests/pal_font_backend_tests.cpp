#include "pal4/pal_font_backend.h"

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

class FontRuntime final : public pal4::IPalFontBackendRuntime {
public:
    std::uint32_t CurrentD3DDevice() override {
        calls.push_back("device");
        return 0x1000u;
    }
    std::int32_t CreateFromDescription(
        std::uint32_t device, std::int32_t height,
        std::int32_t width, std::int32_t weight,
        std::int32_t sixth, std::int32_t fourth,
        const std::array<std::uint8_t, 4>& options,
        std::uint32_t face_name, std::uint32_t font_object) override {
        calls.push_back("create");
        Check(device == 0x1000u && height == 11 && width == 22 &&
                  weight == 33 && sixth == 66 && fourth == 44 &&
                  options == std::array<std::uint8_t, 4>{1, 2, 3, 4} &&
                  face_name == 0x2000u && font_object == 0x3000u,
              "font backend receives IDA parameter order");
        return result;
    }
    std::int32_t result{};
    std::vector<std::string> calls;
};

class ObjectRuntime final : public pal4::IRenderStateObjectRuntime {
public:
    std::uint32_t CallSlot(std::uint32_t object,
                           std::uint32_t byte_offset) override {
        calls.push_back(byte_offset);
        Check(object == 0x4000u, "virtual slot receives wrapped object");
        if (throw_on_release && byte_offset == 8u) {
            throw std::runtime_error("release");
        }
        return byte_offset + 17u;
    }
    std::uint32_t DrawRectangleSlot(
        std::uint32_t object, std::uint32_t first,
        std::int32_t command, std::int32_t sentinel,
        const std::array<std::int32_t, 4>& rectangle,
        std::int32_t last, std::int32_t penultimate) override {
        Check(object == 0x4000u && first == 0u && command == 12 &&
                  sentinel == -1 &&
                  rectangle == std::array<std::int32_t, 4>{1, 2, 3, 4} &&
                  last == 99 && penultimate == 88,
              "draw slot +0x38 receives wrapped and reordered arguments");
        return 0x1234u;
    }
    bool throw_on_release{};
    std::vector<std::uint32_t> calls;
};

void FontDescriptionForwardsAndConvertsResult() {
    FontRuntime runtime;
    pal4::PalFontDescription description{
        11, 22, 33, 44, 0x2000u, 66, {1, 2, 3, 4}};
    Check(pal4::PalFontBackend::CreateFromDescription(
              0x3000u, description, runtime) &&
              runtime.calls == std::vector<std::string>({
                  "device", "create"}),
          "backend zero is converted to success after D3D device lookup");
    runtime.result = -5;
    Check(!pal4::PalFontBackend::CreateFromDescription(
              0x3000u, description, runtime),
          "backend nonzero is converted to failure");
    pal4::PalFontBackendRuntimeStub stub;
    Check(pal4::PalFontBackend::CreateFromDescription(
              0x3000u, description, stub),
          "font and D3D empty stubs link and run");
}

void RenderStateSlotWrappers() {
    ObjectRuntime runtime;
    Check(pal4::RenderStateObject::GetTextureSlot40(
              0x4000u, runtime) == 0x51u &&
              pal4::RenderStateObject::GetTextureSlot44(
                  0x4000u, runtime) == 0x55u &&
              runtime.calls == std::vector<std::uint32_t>({
                  0x40u, 0x44u}),
          "texture wrappers invoke distinct virtual byte offsets");
    std::uint32_t empty{};
    Check(pal4::RenderStateObject::Release(empty, runtime) == 0u &&
              runtime.calls.size() == 2u,
          "empty release returns zero without a call");
    std::uint32_t object = 0x4000u;
    Check(pal4::RenderStateObject::Release(object, runtime) == 25u &&
              object == 0u && runtime.calls.back() == 8u,
          "release calls slot +8 and clears pointer afterward");
    object = 0x4000u;
    runtime.throw_on_release = true;
    try {
        static_cast<void>(pal4::RenderStateObject::Release(object, runtime));
        Check(false, "release must propagate runtime exception");
    } catch (const std::runtime_error&) {
        Check(object == 0x4000u,
              "throwing release leaves pointer unchanged");
    }
    Check(pal4::RenderStateObject::DrawRectangle(
              0x4000u, 12, {1, 2, 3, 4}, 88, 99,
              runtime) == 0x1234u,
          "rectangle slot result is propagated");
    pal4::RenderStateObjectRuntimeStub stub;
    Check(pal4::RenderStateObject::GetTextureSlot40(
              0x4000u, stub) == 0u,
          "render-state empty stub links");
}

}  // namespace

int main() {
    FontDescriptionForwardsAndConvertsResult();
    RenderStateSlotWrappers();
    std::cout << "2 PAL font backend test cases passed\n";
}
