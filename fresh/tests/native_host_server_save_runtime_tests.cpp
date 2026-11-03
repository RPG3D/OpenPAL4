#include "pal4/native_host_server_save_runtime.h"
#include "pal4/player_manager_object.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Word(const std::span<const std::byte> bytes,
                   const std::size_t offset) {
    Check(offset + sizeof(std::uint32_t) <= bytes.size(),
          "host-server word in range");
    std::uint32_t result{};
    std::memcpy(&result, bytes.data() + offset, sizeof(result));
    return result;
}

void TestEmptyCollections() {
    pal4::PlayerManagerSlotState player{};
    pal4::NativeGameSaveArchiveBuffer archive;
    pal4::NativeHostServerSaveRuntime runtime(archive);
    const auto address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&player));
    Check(runtime.Serialize(address), "empty host-server serialization");
    const std::span bytes(archive.Bytes());
    constexpr auto marker = pal4::HostServerDataCodec::kMarker;
    Check(Word(bytes, 0u) == marker.size() &&
              std::memcmp(bytes.data() + 4u, marker.data(), marker.size()) == 0,
          "host-server marker matches original");
    auto offset = 4u + marker.size();
    for (std::uint32_t index = 0u; index < 11u; ++index) {
        Check(Word(bytes, offset) == 0u, "empty collection count");
        offset += 4u;
    }
    Check(offset == bytes.size(), "empty host-server section length");
}

void TestLiveCollections() {
    pal4::PlayerManagerSlotState player{};
    auto* host = reinterpret_cast<pal4::IndexedCollectionResetState*>(
        reinterpret_cast<std::byte*>(&player) + 0x938u);
    std::array<std::uint32_t, 2> values{37u, 42u};
    std::array<std::uint32_t, 2> pair{7u, 9u};
    const auto values_address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(values.data()));
    const auto pair_address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(pair.data()));
    host->values[0].begin = values_address;
    host->values[0].end = values_address + 8u;
    host->key_values[0].begin = pair_address;
    host->key_values[0].end = pair_address + 8u;

    pal4::NativeGameSaveArchiveBuffer archive;
    pal4::NativeHostServerSaveRuntime runtime(archive);
    Check(runtime.Serialize(static_cast<std::uint32_t>(
              reinterpret_cast<std::uintptr_t>(&player))),
          "live host-server serialization");
    const std::span bytes(archive.Bytes());
    auto offset = 4u + pal4::HostServerDataCodec::kMarker.size();
    Check(Word(bytes, offset) == 2u && Word(bytes, offset + 4u) == 37u &&
              Word(bytes, offset + 8u) == 42u,
          "value count and words");
    offset += 12u;
    Check(Word(bytes, offset) == 1u && Word(bytes, offset + 4u) == 7u &&
              Word(bytes, offset + 8u) == 9u,
          "pair count and words");
    offset += 12u;
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        Check(Word(bytes, offset) == 0u, "remaining collection count");
        offset += 4u;
    }
    Check(offset == bytes.size(), "live host-server section length");
}

}  // namespace

int main() {
    TestEmptyCollections();
    TestLiveCollections();
    std::cout << "PASS: native host-server save runtime\n";
}
