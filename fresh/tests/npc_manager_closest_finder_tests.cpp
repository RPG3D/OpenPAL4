#include "pal4/npc_manager_closest_finder.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(float value) { return std::bit_cast<std::uint32_t>(value); }

struct Runtime final : pal4::NpcManagerClosestRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> distances;
    std::unordered_map<std::uint32_t, std::uint32_t> material_counts;
    std::unordered_map<std::uint32_t, std::uint32_t> missing_next;
    std::unordered_map<std::uint32_t, bool> success;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> calls;
    std::vector<std::uint32_t> clump_calls;
    std::vector<std::uint32_t> missing_calls;
    std::vector<std::vector<float>> magnitude_calls;

    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        writes.emplace_back(address, value);
        words[address] = value;
    }
    bool GetPositionAndData(std::uint32_t npc,
                            std::uint32_t reference,
                            std::uint32_t& data_bits) override {
        Check(reference == 0xa000u, "position reference address");
        calls.emplace_back(npc, data_bits);
        data_bits = distances.at(npc);
        return success.count(npc) == 0u || success.at(npc);
    }
    std::uint32_t CountClumpMaterials(std::uint32_t clump) override {
        clump_calls.push_back(clump);
        return material_counts.at(clump);
    }
    std::uint32_t AdvanceMissingMaterials(
        std::uint32_t node, std::uint32_t header) override {
        Check(header == 0x7000u, "missing-material tree header");
        missing_calls.push_back(node);
        return missing_next.at(node);
    }
    double ApproximateMagnitude(float x, float y, float z) override {
        magnitude_calls.push_back({x, y, z});
        return std::sqrt(x * x + y * y + z * z);
    }
};

pal4::NpcManagerTreeGlobals Globals() { return {1u, 0x6000u}; }

Runtime ThreeNpcs() {
    Runtime runtime;
    runtime.words = {
        {0x502cu, 0x7000u},
        {0x7000u, 0x7100u}, {0x7008u, 0x7300u},
        {0x7100u, 0x6000u}, {0x7104u, 0x7000u},
        {0x7108u, 0x7200u}, {0x711cu, 0x9000u},
        {0x7200u, 0x6000u}, {0x7204u, 0x7100u},
        {0x7208u, 0x7300u}, {0x721cu, 0x9100u},
        {0x7300u, 0x6000u}, {0x7304u, 0x7200u},
        {0x7308u, 0x6000u}, {0x731cu, 0x9200u},
        {0x9654u, 0u}, {0x9754u, 0u}, {0x9854u, 0u},
        {0x91f0u, 1u}, {0x92f0u, 1u}, {0x93f0u, 1u}};
    runtime.distances = {
        {0x9000u, Bits(5.0f)}, {0x9100u, Bits(2.0f)},
        {0x9200u, Bits(2.0f)}};
    return runtime;
}

Runtime ThreeSpatialNpcs() {
    auto runtime = ThreeNpcs();
    runtime.words.insert({
        {0x9014u, 0xc000u}, {0x9114u, 0xc100u},
        {0x9214u, 0xc200u},
        {0xd040u, 1u}, {0xd140u, 1u}, {0xd240u, 1u},
        {0xd044u, Bits(4.0f)}, {0xd144u, Bits(1.0f)},
        {0xd244u, Bits(4.0f)},
        {0x90acu, Bits(4.0f)}, {0x90b0u, Bits(0.0f)},
        {0x90b4u, Bits(0.0f)},
        {0x91acu, Bits(2.0f)}, {0x91b0u, Bits(0.0f)},
        {0x91b4u, Bits(0.0f)},
        {0x92acu, Bits(-4.0f)}, {0x92b0u, Bits(0.0f)},
        {0x92b4u, Bits(0.0f)},
        {0xa000u, Bits(0.0f)}, {0xa004u, Bits(0.0f)},
        {0xa008u, Bits(0.0f)}});
    runtime.words[0x9654u] = 0xd000u;
    runtime.words[0x9754u] = 0xd100u;
    runtime.words[0x9854u] = 0xd200u;
    runtime.material_counts = {
        {0xc000u, 1u}, {0xc100u, 1u}, {0xc200u, 1u}};
    return runtime;
}

void EmptyTree() {
    Runtime runtime;
    runtime.words = {{0x502cu, 0x7000u}, {0x7000u, 0x7000u}};
    Check(pal4::NpcManagerClosestFinder::Find(
              0x5000u, 0xa000u, Bits(1.0f), true,
              Globals(), runtime) == 0u &&
          runtime.calls.empty() && runtime.writes.empty(),
          "empty tree returns zero without touching NPCs");
}

void StrictMinimumAndMark() {
    auto runtime = ThreeNpcs();
    Check(pal4::NpcManagerClosestFinder::Find(
              0x5000u, 0xa000u, Bits(1.0f), true,
              Globals(), runtime) == 0x9100u,
          "equal distance preserves first matching NPC");
    Check(runtime.calls ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x9000u, Bits(1.0f)}, {0x9100u, Bits(5.0f)},
                  {0x9200u, Bits(2.0f)}},
          "data argument is updated in place across callbacks");
    Check(runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x9200u, 0u}, {0x9300u, 0u},
                  {0x9400u, 0u}, {0x9300u, 1u}},
          "every marker is cleared before the selected marker is set");
}

void ActivityAndRecordGate() {
    auto runtime = ThreeNpcs();
    runtime.words[0x91f0u] = 0u;
    runtime.words[0x9754u] = 0xb000u;
    runtime.words[0xb040u] = 0u;
    runtime.words[0x9854u] = 0xb100u;
    runtime.words[0xb140u] = 1u;
    Check(pal4::NpcManagerClosestFinder::Find(
              0x5000u, 0xa000u, 0u, false,
              Globals(), runtime) == 0x9200u &&
          runtime.calls ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x9200u, 0u}} &&
          runtime.writes.size() == 3u,
          "zero data applies activity and record gate without final mark");
}

void OriginalGateAndFloatEdges() {
    auto runtime = ThreeNpcs();
    runtime.words[0x91f0u] = 0u;
    runtime.words[0x92f0u] = 0u;
    runtime.distances[0x9000u] = 0u;
    runtime.distances[0x9100u] = Bits(3.0f);
    runtime.distances[0x9200u] = Bits(1.0f);
    Check(pal4::NpcManagerClosestFinder::Find(
              0x5000u, 0xa000u, Bits(1.0f), false,
              Globals(), runtime) == 0x9000u &&
          runtime.calls.size() == 3u &&
          runtime.calls[1].second == 0u,
          "gate uses original nonzero bits even after callback writes zero");

    auto unordered = ThreeNpcs();
    unordered.distances[0x9000u] = Bits(
        std::numeric_limits<float>::quiet_NaN());
    unordered.distances[0x9100u] = 0x7f7fffffu;
    unordered.distances[0x9200u] = Bits(
        -std::numeric_limits<float>::infinity());
    Check(pal4::NpcManagerClosestFinder::Find(
              0x5000u, 0xa000u, Bits(1.0f), false,
              Globals(), unordered) == 0x9200u,
          "unordered and equal-to-initial-maximum values do not win");

    auto failed = ThreeNpcs();
    failed.success[0x9000u] = false;
    failed.distances[0x9100u] = Bits(4.0f);
    failed.distances[0x9200u] = Bits(6.0f);
    Check(pal4::NpcManagerClosestFinder::Find(
              0x5000u, 0xa000u, Bits(1.0f), false,
              Globals(), failed) == 0x9100u &&
          failed.calls[1].second == Bits(5.0f),
          "failed helper may still update the next call's data input");
}

void SpatialDistanceAndRadius() {
    auto runtime = ThreeSpatialNpcs();
    Check(pal4::NpcManagerClosestFinder::FindByCoordinate(
              0x5000u, 0xa000u, true, Globals(), runtime) == 0x9000u,
          "radius includes equality and later equal distance does not replace");
    Check(runtime.magnitude_calls ==
              std::vector<std::vector<float>>{
                  {4.0f, 0.0f, 0.0f},
                  {2.0f, 0.0f, 0.0f},
                  {-4.0f, 0.0f, 0.0f}} &&
          runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x9200u, 0u}, {0x9300u, 0u},
                  {0x9400u, 0u}, {0x9200u, 1u}},
          "spatial deltas, threshold and marker order");
}

void SpatialMaterialAndActivityGates() {
    auto runtime = ThreeSpatialNpcs();
    runtime.material_counts[0xc000u] = 0u;
    runtime.missing_next[0x7100u] = 0x7200u;
    runtime.words[0x92f0u] = 0u;
    runtime.words[0xd240u] = 0u;
    Check(pal4::NpcManagerClosestFinder::FindByCoordinate(
              0x5000u, 0xa000u, false, Globals(), runtime) == 0u &&
          runtime.missing_calls == std::vector<std::uint32_t>{0x7100u} &&
          runtime.clump_calls ==
              std::vector<std::uint32_t>{0xc000u, 0xc100u, 0xc200u} &&
          runtime.magnitude_calls.empty() &&
          runtime.writes.size() == 3u,
          "missing material advances with helper; inactive and record gate skip distance");

    auto unordered = ThreeSpatialNpcs();
    unordered.words[0xd044u] = Bits(
        std::numeric_limits<float>::quiet_NaN());
    unordered.words[0xd144u] = Bits(4.0f);
    unordered.words[0xd244u] = Bits(15.0f);
    Check(pal4::NpcManagerClosestFinder::FindByCoordinate(
              0x5000u, 0xa000u, false, Globals(), unordered) == 0x9100u,
          "unordered radius fails while an equal radius passes");
}

void RadiusUsesDistanceNotDistanceSquared() {
    auto runtime = ThreeSpatialNpcs();
    runtime.words[0x90acu] = Bits(3.0f);
    runtime.words[0x90b4u] = Bits(4.0f);
    runtime.words[0xd044u] = Bits(5.0f);
    runtime.words[0x92f0u] = 0u;
    runtime.words[0x93f0u] = 0u;
    Check(pal4::NpcManagerClosestFinder::FindByCoordinate(
              0x5000u, 0xa000u, false, Globals(), runtime) == 0x9000u &&
          runtime.magnitude_calls ==
              std::vector<std::vector<float>>{{3.0f, 0.0f, 4.0f}},
          "NPC radius compares approximate distance, not squared distance");
}

void CandidateListPreservesOriginalFilter() {
    const std::vector<std::uint32_t> order{0x9000u,0x9100u,0x9200u};
    auto tree=ThreeSpatialNpcs();auto list=tree;
    const auto original=pal4::NpcManagerClosestFinder::FindByCoordinate(
        0x5000u,0xa000u,true,Globals(),tree);
    Check(pal4::NpcManagerClosestFinder::FindByCoordinateList(
              order,0xa000u,true,list)==original && list.writes==tree.writes &&
          list.clump_calls==tree.clump_calls && list.magnitude_calls==tree.magnitude_calls,
          "list must preserve original filtering, callbacks and marking");
    list=ThreeSpatialNpcs();list.material_counts[0xc000u]=0;
    Check(pal4::NpcManagerClosestFinder::FindByCoordinateList(
              order,0xa000u,false,list)==0x9200u && list.missing_calls.empty(),
          "list skips unavailable model without invoking private tree traversal");
    list=ThreeSpatialNpcs();
    const std::vector<std::uint32_t> reversed{0x9200u,0x9100u,0x9000u};
    Check(pal4::NpcManagerClosestFinder::FindByCoordinateList(
              reversed,0xa000u,false,list)==0x9200u,
          "list preserves supplied order on equal distance");
}

}  // namespace

int main() {
    CandidateListPreservesOriginalFilter();
    EmptyTree();
    StrictMinimumAndMark();
    ActivityAndRecordGate();
    OriginalGateAndFloatEdges();
    SpatialDistanceAndRadius();
    SpatialMaterialAndActivityGates();
    RadiusUsesDistanceNotDistanceSquared();
    std::cout << "NPC closest-finder cases passed\n";
}
