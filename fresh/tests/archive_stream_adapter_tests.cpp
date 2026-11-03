#include "pal4/archive_stream_adapter.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IArchiveStreamRuntime {
public:
    struct Call {
        std::uint32_t stream;
        std::uint32_t slot;
        std::uintptr_t buffer;
        std::uint32_t bytes;
        bool operator==(const Call&) const = default;
    };
    std::vector<Call> calls;

    std::uint32_t InvokeVirtual(std::uint32_t stream,
                                std::uint32_t slot,
                                std::uintptr_t buffer,
                                std::uint32_t bytes) override {
        calls.push_back({stream, slot, buffer, bytes});
        return slot == 16u ? 0x12345678u : 0x87654321u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::ArchiveStreamAdapter adapter(runtime);
    pal4::ArchiveStreamAdapterState state{
        0xdeadbeefu, 0xffu, {0xa1u, 0xb2u, 0xc3u}};
    Check(&adapter.Construct(state, 0x1000u, 0u) == &state &&
              state.stream == 0x1000u && state.mode == 0u &&
              state.padding == std::array<std::uint8_t, 3>{
                  0xa1u, 0xb2u, 0xc3u},
          "constructor writes stream and mode without touching padding");
    std::array<std::uint8_t, 4> write_buffer{1u, 2u, 3u, 4u};
    std::array<std::uint8_t, 8> read_buffer{};
    Check(adapter.Write(state, write_buffer.data(), 4u) == 0x12345678u &&
              adapter.Read(state, read_buffer.data(), 8u) == 0x87654321u &&
              runtime.calls == std::vector<Runtime::Call>{
                  {0x1000u, 16u,
                   reinterpret_cast<std::uintptr_t>(write_buffer.data()), 4u},
                  {0x1000u, 12u,
                   reinterpret_cast<std::uintptr_t>(read_buffer.data()), 8u}},
          "write/read forward virtual slot, buffer, byte count and result");

    runtime.calls.clear();
    static_cast<void>(adapter.Construct(state, 0x2000u, 1u));
    Check(state.mode == 1u &&
              adapter.Read(state, read_buffer.data(), 8u) == 0x87654321u &&
              runtime.calls == std::vector<Runtime::Call>{{
                  0x2000u, 12u,
                  reinterpret_cast<std::uintptr_t>(read_buffer.data()), 8u}},
          "read-mode adapter uses new stream and preserves read slot");

    pal4::ArchiveStreamRuntimeStub stub;
    pal4::ArchiveStreamAdapter stub_adapter(stub);
    Check(stub_adapter.Write(state, write_buffer.data(), 4u) == 0u &&
              stub_adapter.Read(state, read_buffer.data(), 8u) == 0u,
          "native archive stub links with defined no-op returns");
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
