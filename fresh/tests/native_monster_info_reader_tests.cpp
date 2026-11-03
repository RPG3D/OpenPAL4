#include "pal4/native_monster_info_reader.h"
#include "pal4/native_package_archive.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

void AppendWord(std::vector<std::uint8_t>& bytes, const std::uint32_t value) {
    for (unsigned shift = 0u; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void AppendText(std::vector<std::uint8_t>& bytes, const char* text) {
    const auto size = std::char_traits<char>::length(text);
    AppendWord(bytes, static_cast<std::uint32_t>(size + 1u));
    bytes.insert(bytes.end(), text, text + size);
    bytes.push_back(0u);
}

}  // namespace

int main(const int argc, char** argv) {
    try {
        pal4::NativeMonsterInfoReader reader;
        Check(!reader.Parse({}) && reader.Records().empty(),
              "truncated monster header is rejected");
        const std::vector<std::uint8_t> empty(4u);
        Check(reader.Parse(empty) && reader.Records().empty(),
              "empty monster scene has no records");
        const std::vector<std::uint8_t> bad_count{1u, 0u, 0u, 0u};
        Check(!reader.Parse(bad_count) && reader.Records().empty(),
              "monster count must fit the stream");

        pal4::BinaryXmlNode behavior;
        behavior.name = "MSTINFO_Behavior";
        pal4::BinaryXmlNode cache;
        cache.name = "MSTINFO_BufferCache";
        const auto behavior_bytes = pal4::BinaryXmlConfig::Serialize(behavior);
        const auto cache_bytes = pal4::BinaryXmlConfig::Serialize(cache);
        Check(behavior_bytes && cache_bytes,
              "synthetic monster XML documents serialize");

        std::vector<std::uint8_t> bytes;
        AppendWord(bytes, 1u);
        AppendText(bytes, "m01-01-01");
        AppendText(bytes, "401");
        for (std::uint32_t index = 1u; index <= 10u; ++index)
            AppendWord(bytes, index);
        AppendWord(bytes, 2u);
        AppendWord(bytes, 0x1234u);
        AppendWord(bytes, 0x5678u);
        bytes.insert(bytes.end(), behavior_bytes->begin(), behavior_bytes->end());
        bytes.insert(bytes.end(), cache_bytes->begin(), cache_bytes->end());
        Check(reader.Parse(bytes) && reader.Records().size() == 1u &&
                  reader.Records()[0].name == "m01-01-01" &&
                  reader.Records()[0].resource == "401" &&
                  reader.Records()[0].position_bits ==
                      std::array<std::uint32_t, 3>{1u, 2u, 3u} &&
                  reader.Records()[0].rotation_bits ==
                      std::array<std::uint32_t, 3>{4u, 5u, 6u} &&
                  reader.Records()[0].fields ==
                      std::array<std::uint32_t, 5>{7u, 8u, 9u, 10u, 2u} &&
                  reader.Records()[0].values ==
                      std::vector<std::uint32_t>{0x1234u, 0x5678u} &&
                  reader.Records()[0].behavior.Root().name == behavior.name &&
                  reader.Records()[0].buffer_cache.Root().name == cache.name,
              "monster record uses the rewritten reader and XML parser");
        bytes.pop_back();
        Check(!reader.Parse(bytes) && reader.Records().empty(),
              "truncated monster XML does not leave stale records");

        if (argc == 2) {
            const auto root = std::filesystem::path(argv[1]);
            Check(reader.Load(root, "Q01", "N01"),
                  "read original Q01/N01 monster scene member");
            std::printf("Q01/N01 monsters: %zu\n", reader.Records().size());
            for (const auto& record : reader.Records())
                Check(!record.name.empty() && !record.resource.empty() &&
                          !record.behavior.Root().name.empty() &&
                          !record.buffer_cache.Root().name.empty(),
                      "original monster record and XND documents decode");
            Check(reader.Load(root, "M06", "3A") &&
                      reader.Records().size() == 3u,
                  "original M06/3A has three monster records");
            const auto& monsters = reader.Records();
            Check(monsters[0].name == "m06-02-05" &&
                      monsters[0].resource == "422" &&
                      monsters[0].values.size() == 3u &&
                      monsters[1].name == "m06-02-07" &&
                      monsters[1].resource == "421" &&
                      monsters[1].values.size() == 4u &&
                      monsters[2].name == "m06-05-02" &&
                      monsters[2].resource == "423" &&
                      monsters[2].values.size() == 3u,
                  "original monster names, actors and value arrays decode");
            for (const auto& monster : monsters)
                Check(monster.behavior.Root().name == "MSTINFO_Behavior" &&
                          monster.buffer_cache.Root().name ==
                              "MSTINFO_BufferCache",
                      "original monster XND documents decode");
            pal4::NativePackageArchive scenes;
            const auto package = (root / "gamedata" / "scenedata.cpk").string();
            Check(scenes.Open(package.c_str()) &&
                      scenes.Find("M06\\4\\mstInfo.mst") < 0,
                  "original M06/4 intentionally has no monster member");
            Check(!reader.Load(root, "M06", "4") &&
                      reader.SceneMemberMissing() && reader.Records().empty(),
                  "absent scene is distinguished from decoding failure and clears old records");
            Check(!reader.Parse(bad_count) && !reader.SceneMemberMissing(),
                  "malformed data never retains absent-member status");
            Check(reader.Load(root, "M06", "3A") &&
                      !reader.SceneMemberMissing() && reader.Records().size() == 3u,
                  "populated scene reload clears absent-member status");
            std::printf("M06/4 pathway present: %d\n",
                        scenes.Find("M06\\4\\pathway.pwy") >= 0);
            Check(!reader.Load(root, "Q01/other", "N01") &&
                      reader.Records().empty() && !reader.SceneMemberMissing(),
                  "invalid monster scene path clears prior records");
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: unexpected exception: %s\n", error.what());
        return EXIT_FAILURE;
    }
}
