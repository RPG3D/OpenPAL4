#include "pal4/debug_text_collection.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IDebugTextRuntime {
public:
    std::vector<std::string> events;
    std::vector<std::uint32_t> elements;
    std::int32_t property = 0;
    std::size_t index = 0;
    std::uint32_t draw_count = 0;
    bool throw_on_insert = false;

    void InitializeTemporary(pal4::StringBufferState& text) override {
        events.emplace_back("temp-init");
        text = {0xa000u, 0u, 32u};
    }
    void AssignTemporary(pal4::StringBufferState& text,
                         const char* source,
                         const std::uint32_t length) override {
        events.emplace_back("temp-assign");
        Check(std::string(source) == "Hello" && length == 5u,
              "text source and strlen");
        text.length = length;
    }
    void Insert(const std::uint32_t collection,
                const pal4::DebugTextRecord& record) override {
        events.emplace_back("insert");
        Check(collection == 0x5555u && record.text.data == 0xa000u &&
                  record.text.length == 5u &&
                  record.attributes ==
                      std::array<std::uint32_t, 3>{7u, 8u, 9u},
              "temporary record passed to collection insert");
        if (throw_on_insert) {
            throw std::runtime_error("insertion failed");
        }
    }
    std::uint32_t CleanupTemporary(
        pal4::StringBufferState& text) override {
        events.emplace_back("temp-cleanup");
        Check(text.data == 0xa000u, "temporary string is still owned");
        text.data = 0u;
        return 0x1234u;
    }
    std::int32_t GetAndClearProperty(
        const std::uint32_t collection) override {
        events.emplace_back("property");
        Check(collection == 0x5555u, "property collection address");
        const auto old = property;
        property = 0;
        return old;
    }
    std::uint32_t ClearCollection(
        const std::uint32_t collection) override {
        events.emplace_back("clear");
        Check(collection == 0x5555u, "clear collection address");
        return 0xcafeu;
    }
    void InitializeStaticString(const std::uint32_t address) override {
        events.emplace_back("static-init");
        Check(address == 0x8e3dccu, "static string address");
    }
    void RegisterCleanup(const std::uint32_t callback) override {
        events.emplace_back("register-cleanup");
        Check(callback == 0x409fd0u, "static cleanup callback address");
    }
    std::uint32_t FirstElement(
        const std::uint32_t collection) override {
        events.emplace_back("first");
        Check(collection == 0x5555u, "first element collection");
        index = 0;
        return elements.empty() ? 0u : elements.front();
    }
    std::uint32_t AdvanceElement(
        const std::uint32_t collection) override {
        events.emplace_back("advance");
        Check(collection == 0x5555u, "advance element collection");
        ++index;
        return index < elements.size() ? elements[index] : 0u;
    }
    std::uint32_t ReadTextPointer(const std::uint32_t element) override {
        events.emplace_back("text-pointer");
        Check(element == 0x1000u || element == 0x2000u,
              "text element address");
        return element + 0x10u;
    }
    std::uint32_t InitializeFontManager() override {
        events.emplace_back("font");
        return 0xfaceu;
    }
    void DrawFontRectangle(
        const std::uint32_t font, const std::uint32_t text,
        const pal4::DebugTextGlobals& rectangle,
        const std::uint32_t flags, const std::uint32_t color,
        const std::uint32_t size) override {
        events.emplace_back("draw");
        Check(font == 0xfaceu && text == elements.at(draw_count) + 0x10u &&
                  rectangle.left == 384 && rectangle.right == 784 &&
                  rectangle.top == static_cast<std::int32_t>(draw_count * 20u) &&
                  rectangle.bottom ==
                      static_cast<std::int32_t>((draw_count + 1u) * 20u) &&
                  flags == 1u && color == 0xff00ff00u && size == 16u,
              "font call arguments and row rectangle");
        ++draw_count;
    }
    void CleanupStaticString(const std::uint32_t address) override {
        events.emplace_back("static-cleanup");
        Check(address == 0x8e3dccu, "static cleanup string address");
    }
};

struct Fixture {
    pal4::GameRenderState game{0x800000u};
    pal4::DebugTextGlobals globals{};
    pal4::DebugTextCollection collection{game, 0x5555u, globals};
    Runtime runtime;
};

void TestAddGateAndTemporary() {
    Fixture f;
    Check(f.collection.Add(nullptr, 7u, 8u, 9u, f.runtime) == 0x800000u &&
              f.runtime.events.empty(),
          "disabled gate does not inspect text or touch collection");
    f.game.Write(0x554u, 1u);
    Check(f.collection.Add("Hello", 7u, 8u, 9u, f.runtime) == 0x1234u,
          "enabled add forwards temporary cleanup result");
    Check(f.runtime.events ==
              std::vector<std::string>{"temp-init", "temp-assign",
                                       "insert", "temp-cleanup"},
          "add call order");

    f.runtime.events.clear();
    f.runtime.throw_on_insert = true;
    bool threw = false;
    try {
        (void)f.collection.Add("Hello", 7u, 8u, 9u, f.runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && f.runtime.events ==
                       std::vector<std::string>{"temp-init", "temp-assign",
                                                "insert", "temp-cleanup"},
          "insertion exception cleans temporary and propagates");
}

void TestConditionalClear() {
    Fixture f;
    f.runtime.property = 0;
    Check(f.collection.ClearIfPending(f.runtime) == 0u &&
              f.runtime.events == std::vector<std::string>{"property"},
          "zero property skips collection clear");
    f.runtime.events.clear();
    f.runtime.property = -2;
    Check(f.collection.ClearIfPending(f.runtime) == 0xfffffffeu &&
              f.runtime.events == std::vector<std::string>{"property"},
          "negative property returns signed bits without clear");
    f.runtime.events.clear();
    f.runtime.property = 2;
    Check(f.collection.ClearIfPending(f.runtime) == 0xcafeu &&
              f.runtime.events ==
                  std::vector<std::string>{"property", "clear"},
          "positive property clears and returns collection result");
}

void TestDrawAndStaticCleanup() {
    Fixture f;
    f.globals.string_guard = 0x80u;
    f.runtime.property = 2;
    f.runtime.elements = {0x1000u, 0x2000u};
    Check(f.collection.Draw(0xdeadu, f.runtime) == 0xcafeu &&
              f.globals.string_guard == 0x81u &&
              f.runtime.draw_count == 2u,
          "draw iterates two rows and preserves guard bits");
    Check(f.runtime.events ==
              std::vector<std::string>{
                  "property", "static-init", "register-cleanup", "first",
                  "text-pointer", "font", "draw", "advance",
                  "text-pointer", "font", "draw", "advance", "clear"},
          "draw initialization, iteration and clear order");
    f.runtime.events.clear();
    f.runtime.elements.clear();
    f.runtime.property = 1;
    Check(f.collection.Draw(0u, f.runtime) == 0xcafeu &&
              f.runtime.events ==
                  std::vector<std::string>{"property", "first", "clear"},
          "already-initialized string is not registered again");

    f.runtime.events.clear();
    f.runtime.property = -1;
    Check(f.collection.Draw(0u, f.runtime) == 0xffffffffu &&
              f.runtime.events == std::vector<std::string>{"property"},
          "nonpositive property skips all drawing work");

    f.runtime.events.clear();
    pal4::DebugTextCollection::CleanupStaticString(f.runtime);
    Check(f.runtime.events ==
              std::vector<std::string>{"static-cleanup"},
          "CRT callback forwards static string cleanup");
}

}  // namespace

int main() {
    TestAddGateAndTemporary();
    TestConditionalClear();
    TestDrawAndStaticCleanup();
    return EXIT_SUCCESS;
}
