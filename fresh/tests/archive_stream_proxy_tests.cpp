#include "pal4/archive_stream_proxy.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IArchiveStreamProxyRuntime {
public:
    std::vector<std::string> events;
    std::uint32_t next_result = 0xdeadbeefu;
    std::uint32_t last_slot = 0xffffffffu;
    std::vector<std::uint32_t> slots;
    std::uintptr_t last_buffer = 0u;
    std::uint32_t last_bytes = 0u;
    const pal4::ArchiveStreamProxyState* state = nullptr;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x4000u, "proxy reads adapter stream word");
        events.emplace_back("adapter");
        return 0x5000u;
    }
    void FreeMemory(std::uint32_t address) override {
        Check(address == 0x3000u, "deleting destructor frees proxy");
        Check(state != nullptr &&
                  state->vtable == pal4::ArchiveStreamProxy::kBaseVtable,
              "base vtable is restored before free");
        events.emplace_back("free");
    }
    std::uint32_t InvokeVirtual(std::uint32_t stream,
                                std::uint32_t slot,
                                std::uintptr_t buffer,
                                std::uint32_t bytes) override {
        Check(stream == 0x5000u, "virtual call receives inner stream");
        last_slot = slot;
        slots.push_back(slot);
        last_buffer = buffer;
        last_bytes = bytes;
        events.emplace_back("virtual");
        return next_result;
    }
};

void Cases() {
    Runtime runtime;
    pal4::ArchiveStreamProxy proxy(runtime);
    pal4::ArchiveStreamProxyState state{0x1111u, 0x2222u};
    runtime.state = &state;
    Check(&proxy.Construct(state, 0x4000u) == &state &&
              state.vtable == pal4::ArchiveStreamProxy::kVtable &&
              state.adapter == 0x4000u && runtime.events.empty(),
          "constructor stores proxy vtable and adapter pointer");

    const std::array<std::uint32_t, 3> results{
        proxy.CallSlot0(state, 0x111u, 3u),
        proxy.CallSlot4(state, 0x222u, 4u),
        proxy.CallSlot8(state, 0x333u, 5u)};
    for (const auto result : results)
        Check(result == 0xdeadbeefu, "virtual return is forwarded whole");
    Check(runtime.slots == std::vector<std::uint32_t>{0u, 4u, 8u} &&
              runtime.last_buffer == 0x333u && runtime.last_bytes == 5u &&
              runtime.events == std::vector<std::string>{
                  "adapter", "virtual", "adapter", "virtual",
                  "adapter", "virtual"},
          "first three slots dereference adapter once and preserve arguments");

    std::uint32_t buffer = 0x12345678u;
    runtime.events.clear();
    Check(proxy.Read(state, &buffer, 4u) == 0xdeadbeefu &&
              runtime.last_slot == 12u &&
              runtime.last_buffer ==
                  reinterpret_cast<std::uintptr_t>(&buffer) &&
              runtime.last_bytes == 4u &&
              runtime.events ==
                  std::vector<std::string>{"adapter", "virtual"},
          "proxy read composes reconstructed adapter slot twelve");
    runtime.events.clear();
    Check(proxy.Write(state, &buffer, 2u) == 0xdeadbeefu &&
              runtime.last_slot == 16u &&
              runtime.last_buffer ==
                  reinterpret_cast<std::uintptr_t>(&buffer) &&
              runtime.last_bytes == 2u &&
              runtime.events ==
                  std::vector<std::string>{"adapter", "virtual"},
          "proxy write composes reconstructed adapter slot sixteen");

    runtime.events.clear();
    Check(proxy.Destroy(state, 0x3000u, 2u) == 0x3000u &&
              state.vtable == pal4::ArchiveStreamProxy::kBaseVtable &&
              state.adapter == 0x4000u && runtime.events.empty(),
          "nondeleting destructor restores base vtable only");
    static_cast<void>(proxy.Construct(state, 0x4000u));
    Check(proxy.Destroy(state, 0x3000u, 3u) == 0x3000u &&
              state.vtable == pal4::ArchiveStreamProxy::kBaseVtable &&
              runtime.events == std::vector<std::string>{"free"},
          "deleting destructor frees after base vtable reset");

    pal4::ArchiveStreamProxyRuntimeStub stub;
    pal4::ArchiveStreamProxy stub_proxy(stub);
    pal4::ArchiveStreamProxyState empty{};
    static_cast<void>(stub_proxy.Construct(empty, 0u));
    Check(stub_proxy.CallSlot0(empty) == 0u &&
              stub_proxy.Read(empty, nullptr, 0u) == 0u,
          "native runtime stub links");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
