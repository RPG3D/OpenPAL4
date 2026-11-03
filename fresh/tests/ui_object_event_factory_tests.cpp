#include "pal4/ui_object_event_factory.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::UiObjectEventFactoryRuntimeStub {
    std::uint32_t allocation = 0x3000u;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "callback allocation has two DWORDs");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x2004u, "callback target comes from source +4");
        calls.emplace_back("read-target");
        return 0x1234u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        if (address == 0x3000u &&
            value == pal4::UiObjectEventFactory::kBaseVtable) {
            calls.emplace_back("base-vtable");
        } else if (address == 0x3004u && value == 0x1234u) {
            calls.emplace_back("target");
        } else if (address == 0x3000u &&
                   value == pal4::UiObjectEventFactory::kDerivedVtable) {
            calls.emplace_back("derived-vtable");
        } else {
            Check(false, "callback object write matches observed fields");
        }
    }
};

struct LifecycleRuntime final : pal4::UiObjectEventFactoryRuntimeStub {
    std::vector<std::string> calls;
    bool forwarded{};

    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x4000u, "deleting destructor frees callback");
        calls.emplace_back("free");
    }
    bool ForwardToFrameManager(std::uint32_t member,
                               std::uint32_t event) override {
        Check(member == 0x4004u && event == 0x6000u,
              "callback forwards member +4 and original event");
        calls.emplace_back("forward");
        return forwarded;
    }
};

std::string Write(std::uint32_t address, std::uint32_t value) {
    return "write:" + std::to_string(address) + ":" +
           std::to_string(value);
}

void CheckCallbackLifecycle() {
    LifecycleRuntime runtime;
    using Callback = pal4::UiObjectEventCallback;
    Check(Callback::Construct(0x4000u, 1u, 2u, 3u, runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x4008u, 1u), Write(0x400cu, 2u),
                  Write(0x4010u, 3u),
                  Write(0x4000u, Callback::kCallbackVtable)},
          "constructor writes three fields then callback vtable");

    runtime.calls.clear();
    Check(Callback::ConstructIntroduction(
              0x4000u, 11u, 22u, 33u, runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x4008u, 11u), Write(0x400cu, 22u),
                  Write(0x4010u, 33u),
                  Write(0x4000u, Callback::kIntroductionVtable)},
          "introduction callback constructor writes fields then vtable");

    runtime.calls.clear();
    Check(!Callback::ForwardEvent(0x4000u, 0x6000u, runtime) &&
              runtime.calls == std::vector<std::string>{"forward"},
          "event thunk preserves a false result");
    runtime.forwarded = true;
    Check(Callback::ForwardEvent(0x4000u, 0x6000u, runtime),
          "event thunk preserves a true result");

    runtime.calls.clear();
    Callback::Destroy(0x4000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              Write(0x4000u, Callback::kBaseVtable)},
          "nondeleting destructor restores base vtable only");

    runtime.calls.clear();
    Check(Callback::DeleteBase(0x4000u, 2u, runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x4000u, Callback::kBaseVtable)},
          "base deleting destructor ignores flags without low bit");
    runtime.calls.clear();
    Check(Callback::DeleteBase(0x4000u, 3u, runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x4000u, Callback::kBaseVtable), "free"},
          "base deleting destructor frees after vtable write");

    runtime.calls.clear();
    Callback::ConstructAlternateBase(0x4000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              Write(0x4000u, Callback::kAlternateBaseVtable)},
          "alternate base constructor writes only its vtable");

    runtime.calls.clear();
    Check(Callback::DeleteAlternateBase(0x4000u, 0u, runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x4000u, Callback::kAlternateBaseVtable)},
          "alternate base destructor writes its own vtable");
    runtime.calls.clear();
    Check(Callback::DeleteAlternateBase(0x4000u, 1u, runtime) == 0x4000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x4000u, Callback::kAlternateBaseVtable), "free"},
          "alternate base deleting destructor conditionally frees");
}

struct LoadRuntime final : pal4::UiObjectEventFactoryRuntimeStub {
    std::uint32_t allocation = 0x5000u;
    std::uint32_t owned = 0x5000u;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "load callback clone allocates 24 bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        if (address == 0x6000u) return owned;
        Check(address == 0x3008u || address == 0x300cu ||
                  address == 0x3010u,
              "load callback clone copies source fields");
        return address + 0x100u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back(Write(address, value));
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "load callback free uses callback address");
        calls.emplace_back("free");
    }
    std::uint32_t CallDeletingDestructor(
        std::uint32_t address, std::uint32_t flags) override {
        Check(address == owned && flags == 1u,
              "owner calls callback virtual deleting destructor");
        calls.emplace_back("virtual-delete");
        return 0x7777u;
    }
};

void CheckLoadCallbackLifecycle() {
    using Callback = pal4::UiObjectEventCallback;
    LoadRuntime runtime;
    Check(Callback::CloneLoad(0x3000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate",
                  Write(0x5000u, Callback::kLoadBaseVtable),
                  "read:12296", Write(0x5008u, 0x3108u),
                  "read:12300", Write(0x500cu, 0x310cu),
                  "read:12304", Write(0x5010u, 0x3110u),
                  Write(0x5000u, Callback::kLoadCallbackVtable)},
          "load callback clone copies three fields between vtable writes");
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(Callback::CloneLoad(0x3000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null allocation skips callback writes");
    runtime.calls.clear();
    Check(Callback::ReleaseOwned(0x6000u, runtime) == 0x7777u &&
              runtime.calls ==
                  (std::vector<std::string>{"read:24576", "virtual-delete"}),
          "owner forwards deleting flag and virtual result");
    runtime.calls.clear();
    runtime.owned = 0u;
    Check(Callback::ReleaseOwned(0x6000u, runtime) == 0u &&
              runtime.calls == (std::vector<std::string>{"read:24576"}),
          "null owner pointer skips virtual destructor");
    runtime.calls.clear();
    Callback::DestroyLoad(0x5000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              Write(0x5000u, Callback::kLoadBaseVtable)},
          "load callback nondeleting destructor restores base vtable");
    runtime.calls.clear();
    Check(Callback::DeleteLoadBase(0x5000u, 2u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x5000u, Callback::kLoadBaseVtable)},
          "deleting destructor ignores flags without low bit");
    runtime.calls.clear();
    Check(Callback::DeleteLoadBase(0x5000u, 3u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x5000u, Callback::kLoadBaseVtable), "free"},
          "deleting destructor frees only after base vtable write");
}

struct FrameRuntime final : pal4::UiObjectEventFactoryRuntimeStub {
    std::uint32_t allocation = 0x5000u;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 8u, "frame callback clone allocates eight bytes");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x3004u || address == 0x5004u,
              "callback handler is at +4");
        calls.emplace_back("read");
        return 0x1234u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back(Write(address, value));
    }
    std::uint32_t InvokeStoredHandler(std::uint32_t handler,
                                      std::uint32_t event) override {
        Check(handler == 0x1234u && event == 0x6789u,
              "stored handler receives original event");
        calls.emplace_back("invoke");
        return 0x99u;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "frame callback frees its own storage");
        calls.emplace_back("free");
    }
};

void CheckFrameCallbacks() {
    using Callback = pal4::UiObjectEventCallback;
    FrameRuntime runtime;
    Check(Callback::CloneFrame(0x3000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", Write(0x5000u, Callback::kBaseVtable),
                  "read", Write(0x5004u, 0x1234u),
                  Write(0x5000u, Callback::kFrameCallbackVtable)},
          "frame callback clone writes base, target and derived vtable");
    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(Callback::CloneFrame(0x3000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "null frame allocation does not read the source");
    runtime.calls.clear();
    Check(Callback::CallStoredHandler(0x5000u, 0x6789u, runtime) == 0x99u &&
              runtime.calls == std::vector<std::string>{"read", "invoke"},
          "stored event handler result is forwarded");
    runtime.calls.clear();
    Check(Callback::DeleteFrame(0x5000u, 2u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x5000u, Callback::kBaseVtable)},
          "nondeleting frame destructor restores base vtable");
    runtime.calls.clear();
    Check(Callback::DeleteFrame(0x5000u, 3u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x5000u, Callback::kBaseVtable), "free"},
          "deleting frame destructor frees after base destruction");
    runtime.calls.clear();
    Check(Callback::ConstructMain(0x5000u, 0x448930u, 0u, 0x900u,
                                  runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  Write(0x5008u, 0x448930u), Write(0x500cu, 0u),
                  Write(0x5010u, 0x900u),
                  Write(0x5000u, Callback::kMainCallbackVtable)},
          "main callback constructor writes handler, zero, owner and vtable");
}

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::UiObjectEventFactory::Create(0x2000u, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "base-vtable", "read-target", "target",
                  "derived-vtable"},
          "event object records target between base and derived vtable writes");

    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::UiObjectEventFactory::Create(0x2000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure does not read or write the object");
    CheckCallbackLifecycle();
    CheckLoadCallbackLifecycle();
    CheckFrameCallbacks();
    std::cout << "UI object event factory tests passed\n";
}
