#include "pal4/native_host_server_load_runtime.h"
#include "pal4/native_host_server_save_runtime.h"
#include "pal4/native_game_save_archive_reader.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/player_manager_object.h"
#include <windows.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::uint32_t Address(const void* pointer) {
    return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pointer));
}
auto Root() {
    return std::filesystem::path("H:/PAL4/objs/host-server-load") /
        std::to_string(GetCurrentProcessId());
}
void Fixture(std::span<const std::byte> bytes) {
    std::filesystem::create_directories(Root() / "save");
    std::ofstream file(Root() / "save/0.dat", std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    Check(static_cast<bool>(file), "write fixture");
}
void Generated() {
    pal4::PlayerManagerSlotState source{}, target{};
    auto& host = *reinterpret_cast<pal4::IndexedCollectionResetState*>(Address(&source) + 0x938u);
    std::array<std::array<std::uint32_t, 6>, 11> payload{};
    auto bind = [&](auto& slot, unsigned index, unsigned words) {
        for (unsigned j = 0; j < words; ++j) payload[index][j] = index * 100u + j + 1u;
        slot.begin = Address(payload[index].data());
        slot.end = slot.capacity_end = slot.begin + words * 4u;
    };
    for (unsigned i = 0; i < 5u; ++i) {
        bind(host.values[i], i * 2u, 3u);
        bind(host.key_values[i], i * 2u + 1u, 6u);
    }
    bind(host.key_values[5], 10u, 3u);
    pal4::NativeGameSaveArchiveBuffer output;
    pal4::NativeHostServerSaveRuntime save(output);
    Check(save.Serialize(Address(&source)), "serialize generated HostServer");
    const auto expected = output.Bytes();
    Fixture(expected);
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeGameSaveArchiveReader input;
    const auto restore = [&] {
        Check(input.OpenSlot(Root(), 0), "open generated fixture");
        Check(pal4::NativeHostServerLoadRuntime::Deserialize(Address(&target), memory, input), "restore host collections");
        Check(input.Remaining() == 0u, "exact host cursor");
        input.Close();
        pal4::NativeGameSaveArchiveBuffer restored;
        pal4::NativeHostServerSaveRuntime serialize(restored);
        Check(serialize.Serialize(Address(&target)) && restored.Bytes() == expected,
              "all eleven arrays roundtrip byte for byte");
    };
    restore();
    Check(memory.OutstandingAllocations() == 11u, "all arrays use player memory owner");
    restore();
    Check(memory.OutstandingAllocations() == 11u, "repeat restore reuses capacities");
    auto bad = expected;
    bad[4] = std::byte{'X'};
    Fixture(bad);
    Check(input.OpenSlot(Root(), 0) &&
        !pal4::NativeHostServerLoadRuntime::Deserialize(Address(&target), memory, input),
        "wrong flag rejects before mutations");
    input.Close();
    pal4::NativeGameSaveArchiveBuffer unchanged;
    pal4::NativeHostServerSaveRuntime check(unchanged);
    Check(check.Serialize(Address(&target)) && unchanged.Bytes() == expected, "wrong flag preserves collections");
    // Reject count multiplication overflow before shared reserve can wrap.
    bad = expected;
    const auto count_offset = 4u + pal4::HostServerDataCodec::kMarker.size();
    const std::uint32_t overflow = 0x40000000u;
    std::memcpy(bad.data() + count_offset, &overflow, 4u);
    Fixture(bad);
    Check(input.OpenSlot(Root(), 0), "open overflow");
    bool failed = false;
    try { (void)pal4::NativeHostServerLoadRuntime::Deserialize(Address(&target), memory, input); }
    catch (const std::runtime_error&) { failed = true; }
    Check(failed && memory.OutstandingAllocations() == 11u, "overflow rejected without allocation");
    input.Close();
    Fixture(std::span(expected).first(expected.size() - 1u));
    Check(input.OpenSlot(Root(), 0), "open truncated payload");
    failed = false;
    try { (void)pal4::NativeHostServerLoadRuntime::Deserialize(Address(&target), memory, input); }
    catch (const std::runtime_error&) { failed = true; }
    Check(failed, "truncated final word array fails");
    input.Close();
    Fixture(expected);
    restore();
    // Slot destruction uses the same owner after this transient reader ends.
    auto& loaded = *reinterpret_cast<pal4::IndexedCollectionResetState*>(Address(&target) + 0x938u);
    for (auto& slot : loaded.values) { (void)memory.Free(slot.begin); slot = {}; }
    for (auto& slot : loaded.key_values) { (void)memory.Free(slot.begin); slot = {}; }
    Check(memory.OutstandingAllocations() == 0u, "release restored buffers");
}
void Original(const std::filesystem::path& root) {
    std::ifstream file(root / "save/0.dat", std::ios::binary);
    Check(static_cast<bool>(file), "open original save fixture");
    const std::string bytes{std::istreambuf_iterator<char>(file), {}};
    pal4::NativeGameSaveArchiveBuffer marker;
    Check(marker.WriteMarker(pal4::HostServerDataCodec::kMarker), "encode marker");
    const std::string needle(reinterpret_cast<const char*>(marker.Bytes().data()), marker.Bytes().size());
    std::size_t next = 0u;
    unsigned sections = 0u;
    // Fixture-only section discovery; production continues the same stream
    // immediately after each role's BASIC and Player XML documents.
    while ((next = bytes.find(needle, next)) != std::string::npos) {
        pal4::PlayerManagerSlotState player{};
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeGameSaveArchiveReader input;
        Check(input.OpenSlot(root, 0), "open original continuous reader");
        std::vector<std::byte> prefix(next);
        Check(input.ReadBytes(prefix), "advance to original host section");
        Check(pal4::NativeHostServerLoadRuntime::Deserialize(Address(&player), memory, input),
              "restore original host section");
        pal4::NativeGameSaveArchiveBuffer output;
        pal4::NativeHostServerSaveRuntime save(output);
        Check(save.Serialize(Address(&player)), "reserialize original host section");
        Check(input.Position() == next + output.Bytes().size() &&
            std::memcmp(bytes.data() + next, output.Bytes().data(), output.Bytes().size()) == 0,
            "original HostServer exact cursor and byte roundtrip");
        next = static_cast<std::size_t>(input.Position());
        ++sections;
    }
    Check(sections == 4u, "verify all four original player HostServer sections");
}
}  // namespace
int main(int argc, char** argv) {
    try {
        Generated();
        if (argc > 1) Original(argv[1]);
        std::cout << "PASS: native HostServer restoration\n";
    }
    catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
