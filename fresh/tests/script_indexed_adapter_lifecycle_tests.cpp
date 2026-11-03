#include "pal4/script_event_system.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptIndexedAdapterLifecycleRuntime {
    struct FreeCall {
        std::uint32_t address;
        pal4::ScriptIndexedAdapterImage image;
    };
    std::vector<FreeCall> calls;

    void Free(std::uint32_t address,
              const pal4::ScriptIndexedAdapterImage& image) override {
        calls.push_back({address, image});
    }
};

using Image = pal4::ScriptIndexedAdapterImage;
using Lifecycle = pal4::ScriptIndexedAdapterLifecycle;
using Delete = std::uint32_t (*)(Image&, std::uint32_t, std::uint8_t,
                                  pal4::IScriptIndexedAdapterLifecycleRuntime&);

void TestResetMethods() {
    Image reader{0x842b9cu, 0x42f140u, 5u};
    Image writer{0x842be4u, 0x42f160u, 7u};
    Lifecycle::ResetReadBase(reader);
    Lifecycle::ResetWriteBase(writer);
    Check(reader.vtable == Lifecycle::kReadBaseVtable &&
              reader.callback == 0x42f140u && reader.index == 5u &&
              writer.vtable == Lifecycle::kWriteBaseVtable &&
              writer.callback == 0x42f160u && writer.index == 7u,
          "base cleanup changes only the first word");
}

void TestDeletingMethods() {
    const std::array<Delete, 4> methods{
        &Lifecycle::DeleteRead, &Lifecycle::DeleteWrite,
        &Lifecycle::DeleteReadBase, &Lifecycle::DeleteWriteBase};
    const std::array<std::uint32_t, 4> vtables{
        Lifecycle::kReadBaseVtable, Lifecycle::kWriteBaseVtable,
        Lifecycle::kReadBaseVtable, Lifecycle::kWriteBaseVtable};

    for (std::size_t i = 0; i < methods.size(); ++i) {
        for (std::uint8_t flags : {0u, 1u, 2u, 3u}) {
            Image image{0xdeadbeefu, 0x42f160u, 11u};
            Runtime runtime;
            const auto result = methods[i](image, 0x5000u, flags, runtime);
            Check(result == 0x5000u && image.vtable == vtables[i] &&
                      image.callback == 0x42f160u && image.index == 11u,
                  "deleting method returns this and preserves payload");
            Check(runtime.calls.size() == (flags & 1u),
                  "only bit zero requests memory release");
            if ((flags & 1u) != 0u)
                Check(runtime.calls[0].address == 0x5000u &&
                          runtime.calls[0].image.vtable == vtables[i] &&
                          runtime.calls[0].image.callback == 0x42f160u &&
                          runtime.calls[0].image.index == 11u,
                      "vtable reset occurs before memory release");
        }
    }
}

}  // namespace

int main() {
    TestResetMethods();
    TestDeletingMethods();
    Image image{0x842b9cu, 0u, 0u};
    pal4::ScriptIndexedAdapterLifecycleRuntimeStub stub;
    Check(Lifecycle::DeleteRead(image, 0x1000u, 1u, stub) == 0x1000u,
          "default lifecycle runtime links");
    std::cout << "18 script indexed adapter lifecycle cases passed\n";
}
