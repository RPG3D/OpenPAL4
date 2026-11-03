#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameManagerLoadCursorRuntime {
    std::vector<std::string> events;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::uint32_t, std::uint8_t> references;
    std::uint32_t next_address = 0x1000u;
    std::uint32_t found = 0xdeadbeefu;
    std::uint32_t image = 0x12340000u;
    std::uint8_t query_cleanup = 0xa7u;
    bool null_path = false;
    std::uint8_t path_reference = 1u;
    pal4::UiCursorRecord inserted{};

    std::uint8_t SetErrorFlag(pal4::StringBufferState& text,
                              std::uint8_t flag) override {
        events.push_back(flag == 0u ? "init" : "cleanup");
        if (flag == 0u) {
            text = {};
            return 0u;
        }
        if (text.data != 0u) {
            const auto header = text.data - 1u;
            auto& count = references.at(header);
            if (count > 0u && count != 0xffu) --count;
            text = {};
        }
        return query_cleanup;
    }
    void AssignCString(pal4::StringBufferState& text,
                       const char* value) override {
        events.emplace_back(std::string("assign:") + value);
        if (null_path && std::string(value) == "gamedata\\cursor\\")
            return;
        const auto header = next_address;
        next_address += 0x1000u;
        text.data = header + 1u;
        text.length = static_cast<std::uint32_t>(std::string(value).size());
        text.capacity = text.length;
        strings[text.data] = value;
        references[header] = std::string(value) == "gamedata\\cursor\\"
                                 ? path_reference : 1u;
    }
    void AppendCString(pal4::StringBufferState& text,
                       const char* value) override {
        events.emplace_back(std::string("append:") + value);
        if (text.data == 0u) return;
        strings.at(text.data) += value;
        text.length = static_cast<std::uint32_t>(strings.at(text.data).size());
    }
    void AssignString(pal4::StringBufferState& destination,
                      const pal4::StringBufferState& source) override {
        events.emplace_back("copy-name");
        destination = source;
        if (source.data != 0u) ++references.at(source.data - 1u);
    }
    std::uint32_t TreeEnd(std::uint32_t manager) override {
        Check(manager == 0x5000u, "tree owner");
        events.emplace_back("end");
        return 0xdeadbeefu;
    }
    std::uint32_t Find(std::uint32_t manager,
                       const pal4::StringBufferState& name) override {
        Check(manager == 0x5000u && strings.at(name.data) == "talk.ani",
              "lookup key");
        events.emplace_back("find");
        return found;
    }
    void LogMissingCursor(const char* file, std::uint32_t line,
                          const char* format, const char* name) override {
        Check(std::string(file) ==
                  "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\UiFrame\\uiFrameManager.cpp" &&
                  line == 0x82u &&
                  std::string(format) ==
                      "FILE:  [%s]\nLINE:  [%d]\nINFO:  uiCursorManager loadCursor %s OK\n\n\n" &&
                  std::string(name) == "talk.ani",
              "missing cursor log evidence");
        events.emplace_back("log");
    }
    std::uint32_t LoadImage(const pal4::StringBufferState& path,
                            std::uint32_t type, std::uint32_t width,
                            std::uint32_t height,
                            std::uint32_t flags) override {
        Check(type == 2u && width == 32u && height == 32u &&
                  flags == 0x10u,
              "LoadImageA arguments");
        Check(path.data == 0u ||
                  strings.at(path.data) == "gamedata\\cursor\\talk.ani",
              "cursor path");
        events.emplace_back("load-image");
        return image;
    }
    void InsertOrFind(std::uint32_t manager,
                      const pal4::UiCursorRecord& cursor) override {
        Check(manager == 0x5000u &&
                  strings.at(cursor.name.data) == "talk.ani",
              "insert key");
        events.emplace_back("insert");
        inserted = cursor;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read-ref");
        return references.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.emplace_back("write-ref");
        references.at(address) = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.emplace_back("free");
        references.erase(address);
        return 0x79u;
    }
};

void CheckAlreadyLoaded() {
    Runtime runtime;
    runtime.found = 0x6000u;
    Check(pal4::UiFrameManagerSingleton::LoadCursor(
              0x5000u, "talk.ani", runtime) == 0xa7u,
          "hit returns query cleanup byte");
    Check(runtime.events ==
              std::vector<std::string>{"init", "assign:talk.ani",
                                       "end", "find", "cleanup"},
          "hit stops after query cleanup");
}

void CheckMissing(std::uint32_t image, std::uint8_t path_reference,
                  std::uint8_t expected_return, bool freed) {
    Runtime runtime;
    runtime.image = image;
    runtime.path_reference = path_reference;
    Check(pal4::UiFrameManagerSingleton::LoadCursor(
              0x5000u, "talk.ani", runtime) == expected_return,
          "path cleanup return byte");
    Check(runtime.inserted.image == image,
          "insert even when image load returns null");
    Check(runtime.events == std::vector<std::string>{
              "init", "assign:talk.ani", "end", "find", "cleanup",
              "log", "init", "assign:gamedata\\cursor\\",
              "append:talk.ani", "load-image", "init",
              "assign:talk.ani", "init", "copy-name", "insert",
              "cleanup", "read-ref", "write-ref", "read-ref",
              freed ? "free" : "write-ref"},
          "miss call order and reference cleanup");
    Check(runtime.references.at(0x3000u) == 0u,
          "source name reference decremented");
    if (!freed)
        Check(runtime.references.at(0x2000u) ==
                  static_cast<std::uint8_t>(path_reference - 1u),
              "path reference decremented");
}

void CheckNullPath() {
    Runtime runtime;
    runtime.null_path = true;
    Check(pal4::UiFrameManagerSingleton::LoadCursor(
              0x5000u, "talk.ani", runtime) == 0u,
          "null path returns zero low byte");
    Check(runtime.events.back() == "write-ref",
          "source name released with no path buffer");
}

}  // namespace

int main() {
    try {
        CheckAlreadyLoaded();
        CheckMissing(0x12340000u, 2u, 0u, false);
        CheckMissing(0u, 0u, 0x79u, true);
        CheckMissing(0u, 0xffu, 0x79u, true);
        CheckNullPath();
        std::cout << "ui_frame_manager_load_cursor_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
