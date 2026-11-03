#include "pal4/base_object_atomic_properties.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
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

struct Record {
    std::uint32_t type{};
    std::string text;
};

struct Runtime final : pal4::IBaseObjectAtomicRuntime {
    std::unordered_map<std::uint32_t, std::vector<Record>> records;
    std::vector<std::uint32_t> atomics{0x1000u, 0x1100u};
    std::vector<std::uint32_t> callback_results;
    std::vector<std::pair<char, std::uint32_t>> events;

    std::uint32_t ForAllAtomics(
        std::uint32_t clump,
        const std::function<std::uint32_t(std::uint32_t)>& callback) override {
        Check(clump == 0x2000u, "enumeration uses the body clump");
        events.emplace_back('E', clump);
        for (auto atomic : atomics) {
            const auto result = callback(atomic);
            callback_results.push_back(result);
            if (result == 0u) break;
        }
        return 0xdeadbeefu;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back('D', address);
        if (address == 0x1018u) return 0x6000u;
        if (address == 0x1118u) return 0x6800u;
        if (address == 0x6020u || address == 0x6820u) return 0x8000u;
        if (address == 0x8000u) return 0x9000u;
        if (address == 0x9000u) return 0xa000u;
        for (const auto& [collection, entries] : records) {
            const auto first = collection + 0x104u;
            if (address >= first && address < first + entries.size() * 0x10u &&
                (address - first) % 0x10u == 0u) {
                return entries[(address - first) / 0x10u].type;
            }
        }
        Check(false, "read stays within observed atomic and property fields");
        return 0u;
    }
    std::int32_t UsedCount(std::uint32_t collection) override {
        events.emplace_back('N', collection);
        return static_cast<std::int32_t>(records[collection].size());
    }
    std::uint32_t RecordAt(std::uint32_t collection,
                           std::int32_t index) override {
        Check(index >= 0 &&
                  static_cast<std::size_t>(index) < records[collection].size(),
              "record index stays within used count");
        events.emplace_back('I', static_cast<std::uint32_t>(index));
        return collection + 0x100u + static_cast<std::uint32_t>(index) * 0x10u;
    }
    std::string_view RecordText(std::uint32_t record,
                                std::int32_t index) override {
        const auto collection = record >= 0x6900u ? 0x6800u : 0x6000u;
        Check(record == collection + 0x100u +
                            static_cast<std::uint32_t>(index) * 0x10u,
              "text comes from the indexed record");
        events.emplace_back('T', static_cast<std::uint32_t>(index));
        return records[collection][index].text;
    }
    std::uint32_t StringLength(std::uint32_t address) override {
        Check(address == 0x3000u || address == 0xa010u,
              "property setter measures object and suffix strings");
        events.emplace_back('L', address);
        return address == 0x3000u ? 6u : 4u;
    }
    void InitString(pal4::BaseObjectState&, std::uint32_t offset,
                    std::uint32_t source, std::uint32_t length) override {
        Check(source == 0x3000u && length == 6u &&
                  (offset == 0x544u || offset == 0x554u),
              "property text starts with object name");
        events.emplace_back('S', offset);
    }
    std::uint32_t AppendString(pal4::BaseObjectState&,
                               std::uint32_t offset,
                               std::uint32_t source,
                               std::uint32_t length) override {
        Check(source == 0xa010u && length == 4u &&
                  (offset == 0x544u || offset == 0x554u),
              "property text appends its property suffix");
        events.emplace_back('A', offset);
        return 0xbeefu;
    }
};

void CheckPropertySetters() {
    pal4::BaseObjectState state{};
    state.Write(8u, 0x3000u);
    Runtime runtime;
    for (bool face : {true, false}) {
        runtime.events.clear();
        const auto result = face ?
            pal4::BaseObjectAtomicProperties::SetFaceProperty(
                state, 0x9000u, runtime) :
            pal4::BaseObjectAtomicProperties::SetLegProperty(
                state, 0x9000u, runtime);
        const auto string_offset = face ? 0x544u : 0x554u;
        const auto pointer_offset = face ? 0x550u : 0x560u;
        Check(result == 0xbeefu && state.Read(pointer_offset) == 0x9000u,
              "property setters forward append result and store pointer");
        Check(runtime.events ==
                  std::vector<std::pair<char, std::uint32_t>>{
                      {'L', 0x3000u}, {'S', string_offset},
                      {'D', 0x9000u}, {'L', 0xa010u},
                      {'A', string_offset}},
              "property setters preserve string operation order");
    }
}

void CheckCallbackBranches() {
    pal4::BaseObjectState state{};
    state.Write(8u, 0x3000u);
    Runtime runtime;
    Check(pal4::BaseObjectAtomicProperties::ProcessFaceAtomic(
              state, 0x1000u, runtime) == 0x1000u,
          "zero-record callback preserves atomic pointer");
    runtime.records[0x6000u] = {{3u, "FACE"}, {2u, "FACE_1"},
                                {3u, "OTHER"}, {3u, "FACE_2"}};
    Check(pal4::BaseObjectAtomicProperties::ProcessFaceAtomic(
              state, 0x1000u, runtime) == 0u &&
              state.Read(0x550u) == 0x9000u,
          "FACE callback requires type three and a longer matching prefix");
    Check(runtime.events.back() == std::pair<char, std::uint32_t>{'A', 0x544u},
          "FACE match reaches property-string setter");
    runtime.records[0x6000u] = {{3u, "FACE_2"}, {3u, "LEG"},
                                {3u, "LEG_1"}};
    Check(pal4::BaseObjectAtomicProperties::ProcessLegAtomic(
              state, 0x1000u, runtime) == 0u &&
              state.Read(0x560u) == 0x9000u,
          "LEG callback ignores FACE and exact-length LEG before matching");
}

void CheckEnumerators() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    state.Write(0x550u, 0xaaaau);
    state.Write(0x560u, 0xbbbbu);
    Check(pal4::BaseObjectAtomicProperties::ProcessFaceAtomics(
              state, runtime) == 0u &&
              pal4::BaseObjectAtomicProperties::ProcessLegAtomics(
                  state, runtime) == 0u &&
              state.Read(0x550u) == 0xaaaau &&
              state.Read(0x560u) == 0xbbbbu && runtime.events.empty(),
          "missing clump leaves both stored pointers unchanged");

    state.Write(0x14u, 0x2000u);
    state.Write(8u, 0x3000u);
    runtime.records[0x6000u] = {{3u, "OTHER"}};
    runtime.records[0x6800u] = {{3u, "FACE_1"}};
    Check(pal4::BaseObjectAtomicProperties::ProcessFaceAtomics(
              state, runtime) == 0xdeadbeefu &&
              runtime.callback_results ==
                  std::vector<std::uint32_t>{0x1000u, 0u} &&
              state.Read(0x550u) == 0x9000u,
          "FACE enumeration clears stale pointer and stops at second atomic");
    runtime.callback_results.clear();
    runtime.records[0x6000u] = {{3u, "LEG_1"}};
    runtime.records[0x6800u] = {{3u, "OTHER"}};
    Check(pal4::BaseObjectAtomicProperties::ProcessLegAtomics(
              state, runtime) == 0xdeadbeefu &&
              runtime.callback_results == std::vector<std::uint32_t>{0u} &&
              state.Read(0x560u) == 0x9000u,
          "LEG enumeration stops on first matching atomic");
}

}  // namespace

int main() {
    CheckPropertySetters();
    CheckCallbackBranches();
    CheckEnumerators();
}
