#include "pal4/collection_collision_scanner.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICollectionCollisionRuntime {
public:
    const float* expected_query = nullptr;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::optional<pal4::CollectionCollisionHit>> hits;
    std::vector<std::uint32_t> probed;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::optional<pal4::CollectionCollisionHit> Probe(
        const float* query, std::uint32_t object) override {
        Check(query == expected_query, "collision query pointer preserved");
        probed.push_back(object);
        return hits.at(object);
    }
};

void Cases() {
    Runtime runtime;
    pal4::CollectionCollisionScanner scanner(runtime);
    pal4::ScriptUniqueCollectionState state;
    const std::array<float, 3> query{1.0f, 2.0f, 3.0f};
    runtime.expected_query = query.data();
    float limit = 10.0f;
    std::array<std::uint32_t, 3> point{9u, 9u, 9u};
    Check(!scanner.FindNearest(state, query.data(), limit, point) &&
              limit == 10.0f && point == std::array<std::uint32_t, 3>{9u, 9u, 9u} &&
              runtime.probed.empty(),
          "empty collection preserves outputs and skips probes");

    state.At(4u) = 0x1000u;
    state.At(8u) = 0x1014u;
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.words[0x1000u + 4u * index] = 0x2000u + index;
    runtime.hits[0x2000u] = std::nullopt;
    runtime.hits[0x2001u] = pal4::CollectionCollisionHit{8.0f, {1u, 2u, 3u}};
    runtime.hits[0x2002u] = pal4::CollectionCollisionHit{8.0f, {4u, 5u, 6u}};
    runtime.hits[0x2003u] = pal4::CollectionCollisionHit{3.0f, {7u, 8u, 9u}};
    runtime.hits[0x2004u] = pal4::CollectionCollisionHit{
        std::numeric_limits<float>::quiet_NaN(), {10u, 11u, 12u}};
    Check(scanner.FindNearest(state, query.data(), limit, point) &&
              limit == 3.0f &&
              point == std::array<std::uint32_t, 3>{7u, 8u, 9u} &&
              runtime.probed == std::vector<std::uint32_t>{
                  0x2000u, 0x2001u, 0x2002u, 0x2003u, 0x2004u},
          "nearest strict hit wins across misses ties and NaN");

    runtime.probed.clear();
    limit = 3.0f;
    point = {13u, 14u, 15u};
    Check(!scanner.FindNearest(state, query.data(), limit, point) &&
              limit == 3.0f &&
              point == std::array<std::uint32_t, 3>{13u, 14u, 15u} &&
              runtime.probed.size() == 5u,
          "equal nearest distance does not count as an update");
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
