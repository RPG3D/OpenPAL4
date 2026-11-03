#include "pal4/native_game_save_archive_buffer.h"
#include "pal4/native_game_save_summary.h"

#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main(int argc, char** argv) {
    pal4::NativeGameSaveArchiveBuffer buffer;
    pal4::GameSaveHeader header;
    header.game_key = 907u;
    Check(buffer.WritePrefix("TEST", header) &&
              buffer.Bytes().size() == 4u + 4u + sizeof(header),
          "archive prefix uses original text and header writers");
    std::uint32_t length = 0u;
    std::memcpy(&length, buffer.Bytes().data(), sizeof(length));
    Check(length == 4u &&
              std::memcmp(buffer.Bytes().data() + 4u, "TEST", 4u) == 0,
          "archive text starts with its byte length");

    const auto fixture_root = std::filesystem::path("H:/PAL4/objs/archive-reader") /
        std::to_string(GetCurrentProcessId());
    std::filesystem::create_directories(fixture_root / "save");
    pal4::BinaryXmlNode first;
    first.name = "First";
    first.attributes.push_back({"value", 1u, std::int32_t{27}});
    pal4::BinaryXmlNode second;
    second.name = "Second";
    second.attributes.push_back({"name", 3u, std::string("scene")});
    Check(buffer.WriteMarker("GA_SERIALIZE_FLAG") && buffer.WriteConfig(first) &&
            buffer.WriteMarker(std::string(512, 'L')) && buffer.WriteConfig(second) &&
            buffer.WriteMarker(""), "write continuous archive fixture");
    {
        std::ofstream output(fixture_root / "save" / "0.dat", std::ios::binary);
        output.write(reinterpret_cast<const char*>(buffer.Bytes().data()), buffer.Bytes().size());
        Check(static_cast<bool>(output), "write continuous save stream under objs");
    }
    pal4::NativeGameSaveArchiveReader stream;
    pal4::GameSaveHeader loaded;
    Check(stream.OpenSlot(fixture_root, 0) && stream.ReadMarker() == "TEST" && stream.ReadHeader(loaded) &&
            loaded.game_key == 907u && stream.Position() == 60u && stream.ReadMarker() == "GA_SERIALIZE_FLAG",
            "native reader retains cursor after prefix and section marker");
    pal4::BinaryXmlConfig xml;
    Check(stream.ReadConfig(xml) && xml.Root().name == "First" &&
            xml.Root().FindAttribute("value")->ToInt(0u) == 27u && stream.ReadMarker() == std::string(512, 'L') &&
            stream.ReadConfig(xml) && xml.Root().name == "Second" && stream.ReadMarker() == "" && stream.Remaining() == 0u,
            "continuous configs consume exactly their own bytes, preserving following long/empty markers");
    const auto end_position = stream.Position();
    std::uint32_t beyond = 0x11223344u;
    Check(!stream.ReadBytes(std::as_writable_bytes(std::span(&beyond, 1u))) &&
            beyond == 0x11223344u && stream.Position() == end_position,
            "out-of-range read preserves destination and cursor");
    stream.Close();
    Check(!stream.IsOpen() && stream.Position() == 0u && stream.Remaining() == 0u,
            "explicit close releases stream and cursor");
    Check(stream.OpenSlot(fixture_root, 0) && stream.ReadMarker() == "TEST" && stream.ReadHeader(loaded),
            "closed reader can reopen");
    const auto invalid_xml_position = stream.Position();
    Check(!stream.ReadConfig(xml) && stream.Position() == invalid_xml_position && xml.Root().name == "Second" &&
            stream.ReadMarker() == "GA_SERIALIZE_FLAG", "bad XML restores cursor and leaves caller config unchanged");
    Check(!stream.OpenSlot(fixture_root, 99) && !stream.IsOpen() && stream.Position() == 0u,
            "missing reopen closes previous stream");
    if (argc > 1) {
        const std::filesystem::path root(argv[1]);
        pal4::NativeGameSaveSummaryReader reader;
        const auto summary = reader.ReadSlot(root, 0);
        Check(summary.has_value(), "read original slot zero summary");
        Check(buffer.WritePrefix(summary->marker, summary->header),
              "rebuild original slot zero prefix");
        std::ifstream original(root / "save" / "0.dat", std::ios::binary);
        Check(static_cast<bool>(original), "open original save for comparison");
        std::string original_prefix(buffer.Bytes().size(), '\0');
        original.read(original_prefix.data(),
                      static_cast<std::streamsize>(original_prefix.size()));
        Check(original.gcount() ==
                  static_cast<std::streamsize>(buffer.Bytes().size()) &&
                  std::memcmp(original_prefix.data(), buffer.Bytes().data(),
                              buffer.Bytes().size()) == 0,
              "archive prefix matches original save bytes");

        original.clear();
        original.seekg(0);
        const std::vector<std::uint8_t> original_bytes{
            std::istreambuf_iterator<char>(original),
            std::istreambuf_iterator<char>()};
        std::size_t offset = buffer.Bytes().size();
        Check(original_bytes.size() >= offset + 4u,
              "original save has GameData marker");
        std::uint32_t marker_length = 0u;
        std::memcpy(&marker_length, original_bytes.data() + offset, 4u);
        offset += 4u;
        Check(marker_length == sizeof("GA_SERIALIZE_FLAG") - 1u &&
                  original_bytes.size() >= offset + marker_length + 4u &&
                  std::memcmp(original_bytes.data() + offset,
                              "GA_SERIALIZE_FLAG", marker_length) == 0,
              "original GameData marker follows save header");
        offset += marker_length;
        std::uint32_t native_record_count = 0u;
        std::memcpy(&native_record_count,
                    original_bytes.data() + offset, 4u);
        Check(native_record_count == 0u,
              "original slot zero has no GameData native records");
        offset += 4u;
        pal4::BinaryXmlConfig game_data;
        std::size_t config_length = 0u;
        Check(game_data.ParsePrefix(std::span(original_bytes).subspan(offset),
                                    config_length) &&
                  game_data.Root().name == "gameData",
              "parse original GameData config from save stream");
        Check(buffer.WriteMarker("GA_SERIALIZE_FLAG") &&
                  buffer.WriteBytes(std::as_bytes(
                      std::span(&native_record_count, 1u))) &&
                  buffer.WriteConfig(game_data.Root()) &&
                  buffer.Bytes().size() == offset + config_length &&
                  std::memcmp(buffer.Bytes().data(), original_bytes.data(),
                              buffer.Bytes().size()) == 0,
              "rebuild original GameData save segment byte-for-byte");
        offset += config_length;
        Check(original_bytes.size() >= offset + 4u,
              "original save has Inventory marker");
        std::memcpy(&marker_length, original_bytes.data() + offset, 4u);
        offset += 4u;
        Check(marker_length == sizeof("IY_SERIALIZE_FLAG") - 1u &&
                  original_bytes.size() >= offset + marker_length &&
                  std::memcmp(original_bytes.data() + offset,
                              "IY_SERIALIZE_FLAG", marker_length) == 0,
              "original Inventory marker follows GameData");
        offset += marker_length;
        pal4::BinaryXmlConfig inventory;
        config_length = 0u;
        Check(inventory.ParsePrefix(std::span(original_bytes).subspan(offset),
                                    config_length) &&
                  inventory.Root().name == "INVENTORY",
              "parse original Inventory config from save stream");
        Check(buffer.WriteMarker("IY_SERIALIZE_FLAG") &&
                  buffer.WriteConfig(inventory.Root()) &&
                  buffer.Bytes().size() == offset + config_length &&
                  std::memcmp(buffer.Bytes().data(), original_bytes.data(),
                              buffer.Bytes().size()) == 0,
              "rebuild original Inventory config byte-for-byte");
        pal4::NativeGameSaveArchiveReader original_stream;
        pal4::GameSaveHeader original_header;
        std::uint32_t count = 99u;
        pal4::BinaryXmlConfig read_game, read_inventory;
        Check(original_stream.OpenSlot(root, 0) && original_stream.ReadMarker() == summary->marker &&
                original_stream.ReadHeader(original_header) && original_stream.ReadMarker() == "GA_SERIALIZE_FLAG" &&
                original_stream.ReadBytes(std::as_writable_bytes(std::span(&count, 1u))) && count == 0u &&
                original_stream.ReadConfig(read_game) && read_game.Root().name == "gameData" &&
                original_stream.ReadMarker() == "IY_SERIALIZE_FLAG" && original_stream.ReadConfig(read_inventory) &&
                read_inventory.Root().name == "INVENTORY" && original_stream.Position() == offset + config_length &&
                original_stream.Remaining() > 0u,
                "original save prefix, GameData and Inventory read continuously to next subsystem boundary");

    }
    std::cout << "PASS: native save prefix, GameData and Inventory config\n";
}
