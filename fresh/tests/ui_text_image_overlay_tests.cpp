#include "pal4/ui_text_image_overlay.h"
#include "pal4/ui_text_image_animation.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiTextImageOverlayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t font = 0x7100u;
    bool throw_create = false;
    bool throw_lookup = false;
    bool throw_image = false;
    std::uint32_t image = 0x7200u;
    pal4::UiOverlayRect image_rect{};
    pal4::UiOverlayRect text_rect{};
    pal4::UiOverlayRect image_clip{};
    pal4::UiOverlayRect text_clip{};
    float image_alpha = 0.0f;
    float text_alpha = 0.0f;
    float drawn_scale = 0.0f;
    float measured_scale = 0.0f;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read byte " + std::to_string(address));
        return bytes[address];
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read word " + std::to_string(address));
        return words[address];
    }

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("byte " + std::to_string(address));
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("word " + std::to_string(address));
        words[address] = value;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        calls.emplace_back("copy text");
        for (std::size_t i = 0; i < text.size(); ++i)
            bytes[destination + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
    }
    void SetTextErrorFlag(std::uint32_t record,
                          std::uint8_t flag) override {
        calls.emplace_back("flag " + std::to_string(flag));
        Check(record == 0x5004u, "text record offset");
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        calls.emplace_back("reallocate " + std::to_string(length));
        Check(record == 0x5004u, "reallocate text record offset");
        words[record + 4u] = 0xa000u;
        words[record + 12u] = length | 31u;
        bytes[0x9fffu] = 0u;
    }
    std::uint32_t CreateCeguiString(std::u32string_view text) override {
        std::string narrow;
        for (const char32_t character : text) {
            Check(character <= 255u, "CEGUI character zero extension");
            narrow.push_back(static_cast<char>(character));
        }
        calls.emplace_back("create " + narrow);
        if (throw_create) throw std::runtime_error("create failed");
        return 0x9000u;
    }
    std::uint32_t GetFont(std::uint32_t name) override {
        calls.emplace_back("get font");
        Check(name == 0x9000u, "font name handle");
        if (words.find(0x5000u) != words.end())
            Check(words.at(0x5000u) ==
                      pal4::UiTextImageOverlay::kVtable &&
                      words.at(0x5008u) == 0u &&
                      words.at(0x5034u) == 0u &&
                      words.find(0x5014u) == words.end(),
                  "all other fields initialized before font lookup");
        if (throw_lookup) throw std::runtime_error("font lookup failed");
        return font;
    }
    std::uint32_t GetImage(std::uint32_t name,
                           std::string_view image_name) override {
        calls.emplace_back("get image " + std::string(image_name));
        Check(name == 0x9000u, "imageset name handle");
        if (throw_image) throw std::runtime_error("image lookup failed");
        return image;
    }
    void DestroyCeguiString(std::uint32_t name) override {
        calls.emplace_back("destroy name");
        Check(name == 0x9000u, "destroy the created name");
    }
    void CleanupTextBuffer(std::uint32_t address) override {
        calls.emplace_back("cleanup text");
        Check(address == 0x5004u, "cleanup embedded text member");
    }
    std::uint32_t FreeTextAllocation(std::uint32_t address) override {
        calls.emplace_back("free text");
        Check(address == 0x7fffu, "free reference header address");
        return 0xdeadu;
    }
    std::uint32_t RenderWidth() override {
        calls.emplace_back("render width");
        return 1280u;
    }
    std::uint32_t RenderHeight() override {
        calls.emplace_back("render height");
        return 800u;
    }
    std::string ConvertToUtf8(std::uint32_t text_data) override {
        calls.emplace_back("convert utf8");
        Check(text_data == 0x8000u || text_data == 0u,
              "text data pointer forwarded");
        return text_data ? "PAL4" : "";
    }
    float MeasureText(std::uint32_t selected_font, std::string_view utf8,
                      float scale) override {
        calls.emplace_back("measure text");
        Check(selected_font == font && utf8 == "PAL4",
              "measure font and converted text");
        measured_scale = scale;
        return 80.0f;
    }
    void DrawImage(std::uint32_t selected_image,
                   const pal4::UiOverlayRect& bounds,
                   const pal4::UiOverlayRect& clip, float alpha) override {
        calls.emplace_back("draw image");
        Check(selected_image == image, "draw selected image");
        image_rect = bounds;
        image_clip = clip;
        image_alpha = alpha;
    }
    void DrawOverlayText(std::uint32_t selected_font, std::string_view utf8,
                  const pal4::UiOverlayRect& origin,
                  const pal4::UiOverlayRect& clip, float alpha,
                  float scale) override {
        calls.emplace_back("draw text");
        Check(selected_font == font && utf8 == "PAL4",
              "draw font and converted text");
        text_rect = origin;
        text_clip = clip;
        text_alpha = alpha;
        drawn_scale = scale;
    }
};

void CheckConstruct(std::uint8_t seed, std::uint32_t font) {
    Runtime runtime;
    runtime.font = font;
    runtime.words[0x5038u] = 0xaaaaaaaau;
    Check(pal4::UiTextImageOverlay::Construct(
              0x5000u, seed, runtime) == 0x5000u,
          "constructor returns the object address");
    Check(runtime.bytes.at(0x5004u) == seed,
          "stack-derived seed stored at +4");
    for (const auto offset : {8u, 12u, 16u, 24u, 28u, 32u, 48u, 52u})
        Check(runtime.words.at(0x5000u + offset) == 0u,
              "zero-initialized object fields");
    for (const auto offset : {36u, 40u, 44u})
        Check(runtime.words.at(0x5000u + offset) == 0x3f800000u,
              "unit scale and alpha fields");
    Check(runtime.words.at(0x5000u) ==
              pal4::UiTextImageOverlay::kVtable &&
              runtime.words.at(0x5014u) == font &&
              runtime.words.at(0x5038u) == 0xaaaaaaaau,
          "vtable, font and adjacent memory");
    Check(runtime.calls.at(runtime.calls.size() - 4u) == "create system" &&
              runtime.calls.at(runtime.calls.size() - 3u) == "get font" &&
              runtime.calls.at(runtime.calls.size() - 2u) == "word 20500" &&
              runtime.calls.back() == "destroy name",
          "temporary string and font assignment order");
}

void CheckFailures() {
    Runtime create;
    create.throw_create = true;
    bool failed = false;
    try {
        (void)pal4::UiTextImageOverlay::Construct(0x5000u, 1u, create);
    } catch (const std::runtime_error& error) {
        failed = std::string_view(error.what()) == "create failed";
    }
    Check(failed && create.calls.at(create.calls.size() - 2u) ==
              "create system" && create.calls.back() == "cleanup text" &&
              create.words.find(0x5014u) == create.words.end(),
          "string creation failure cleans embedded text only");

    Runtime lookup;
    lookup.throw_lookup = true;
    failed = false;
    try {
        (void)pal4::UiTextImageOverlay::Construct(0x5000u, 2u, lookup);
    } catch (const std::runtime_error& error) {
        failed = std::string_view(error.what()) == "font lookup failed";
    }
    Check(failed && lookup.calls.at(lookup.calls.size() - 3u) ==
              "get font" &&
              lookup.calls.at(lookup.calls.size() - 2u) ==
                  "destroy name" &&
              lookup.calls.back() == "cleanup text" &&
              lookup.words.find(0x5014u) == lookup.words.end(),
          "font lookup failure destroys name then embedded text");
}

std::size_t CallIndex(const Runtime& runtime, std::string_view call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + std::string(call));
}

bool HasCall(const Runtime& runtime, std::string_view call) {
    for (const auto& entry : runtime.calls)
        if (entry == call) return true;
    return false;
}

void CheckTextAndFont() {
    for (const std::uint8_t reference : {0u, 0xffu}) {
        Runtime runtime;
        runtime.words[0x5008u] = 0x8000u;
        runtime.words[0x5010u] = 31u;
        runtime.bytes[0x7fffu] = reference;
        const auto result = pal4::UiTextImageOverlay::SetTextAndFont(
            0x5000u, "abc", "sys\xff", 0x3f000000u, runtime);
        Check(result == 0x3f000000u &&
                  runtime.words.at(0x5008u) == 0x8000u &&
                  runtime.words.at(0x500cu) == 3u &&
                  runtime.bytes.at(0x8000u) == 'a' &&
                  runtime.bytes.at(0x8003u) == 0u &&
                  runtime.words.at(0x5014u) == runtime.font &&
                  runtime.words.at(0x5024u) == 0x3f000000u &&
                  runtime.bytes.at(0x7fffu) == reference,
              "exclusive or sentinel in-place text, font and scale");
        Check(CallIndex(runtime, "copy text") <
                  CallIndex(runtime, "create sys\xff") &&
                  CallIndex(runtime, "create sys\xff") <
                  CallIndex(runtime, "get font") &&
                  CallIndex(runtime, "destroy name") <
                  CallIndex(runtime, "word 20516"),
              "text, CEGUI and scale write order");
    }

    Runtime shared;
    shared.words[0x5008u] = 0x8000u;
    shared.words[0x5010u] = 31u;
    shared.bytes[0x7fffu] = 2u;
    (void)pal4::UiTextImageOverlay::SetTextAndFont(
        0x5000u, "new", "font", 7u, shared);
    Check(CallIndex(shared, "reallocate 3") <
              CallIndex(shared, "copy text") &&
              shared.words.at(0x5008u) == 0xa000u &&
              shared.bytes.at(0xa000u) == 'n' &&
              !HasCall(shared, "flag 1"),
          "shared nonempty text reallocates without pre-reset");
}

void CheckTextBranches() {
    for (const std::uint32_t capacity : {2u, 32u}) {
        Runtime runtime;
        runtime.words[0x5008u] = 0x8000u;
        runtime.words[0x5010u] = capacity;
        (void)pal4::UiTextImageOverlay::SetTextAndFont(
            0x5000u, "abcd", "", 0u, runtime);
        Check(CallIndex(runtime, "flag 1") <
                  CallIndex(runtime, "reallocate 4") &&
                  runtime.words.at(0x5008u) == 0xa000u &&
                  runtime.words.at(0x500cu) == 4u,
              "insufficient or oversized capacity resets before reallocation");
    }

    Runtime shared_empty;
    shared_empty.words[0x5008u] = 0x8000u;
    shared_empty.bytes[0x7fffu] = 2u;
    (void)pal4::UiTextImageOverlay::SetTextAndFont(
        0x5000u, "", "system", 1u, shared_empty);
    Check(shared_empty.bytes.at(0x7fffu) == 1u &&
              shared_empty.words.at(0x5008u) == 0u &&
              CallIndex(shared_empty, "flag 0") <
                  CallIndex(shared_empty, "create system"),
          "shared empty text decrements reference then clears metadata");

    Runtime exclusive_empty;
    exclusive_empty.words[0x5008u] = 0x8000u;
    exclusive_empty.bytes[0x7fffu] = 0xffu;
    (void)pal4::UiTextImageOverlay::SetTextAndFont(
        0x5000u, "", "", 2u, exclusive_empty);
    Check(exclusive_empty.bytes.at(0x7fffu) == 0xffu &&
              exclusive_empty.words.at(0x5008u) == 0u &&
              CallIndex(exclusive_empty, "flag 1") <
                  CallIndex(exclusive_empty, "create "),
          "sentinel empty text uses release flag");
}

void CheckFontFailure() {
    Runtime runtime;
    runtime.words[0x5008u] = 0x8000u;
    runtime.words[0x5010u] = 31u;
    runtime.throw_lookup = true;
    bool failed = false;
    try {
        (void)pal4::UiTextImageOverlay::SetTextAndFont(
            0x5000u, "abc", "fail", 8u, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "destroy name" &&
              runtime.words.find(0x5014u) == runtime.words.end() &&
              runtime.words.find(0x5024u) == runtime.words.end() &&
              runtime.words.at(0x500cu) == 3u,
          "font lookup failure destroys temporary after text assignment");

    Runtime create;
    create.words[0x5008u] = 0x8000u;
    create.words[0x5010u] = 31u;
    create.throw_create = true;
    failed = false;
    try {
        (void)pal4::UiTextImageOverlay::SetTextAndFont(
            0x5000u, "abc", "fail", 8u, create);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && create.calls.back() == "create fail" &&
              create.words.at(0x500cu) == 3u,
          "temporary creation failure preserves completed text assignment");
}

void CheckProperties() {
    Runtime runtime;
    Check(pal4::UiTextImageOverlay::SetPosition(
              0x5000u, 0x11111111u, 0x22222222u, runtime) ==
              0x11111111u &&
              runtime.words.at(0x501cu) == 0x11111111u &&
              runtime.words.at(0x5020u) == 0x22222222u,
          "position fields and return");
    Check(pal4::UiTextImageOverlay::SetAppearance(
              0x5000u, 0x33333333u, 0x44444444u, runtime) ==
              0x33333333u &&
              runtime.words.at(0x5018u) == 0x33333333u &&
              runtime.words.at(0x5028u) == 0x44444444u,
          "appearance fields and return");
}

void CheckImage() {
    Runtime runtime;
    pal4::UiTextImageOverlay::SetImage(
        0x5000u, "Atlas\xff", "Icon", 0x3f800000u, runtime);
    Check(runtime.words.at(0x5018u) == runtime.image &&
              runtime.words.at(0x5028u) == 0x3f800000u &&
              CallIndex(runtime, "create Atlas\xff") <
                  CallIndex(runtime, "get image Icon") &&
              CallIndex(runtime, "get image Icon") <
                  CallIndex(runtime, "word 20504") &&
              CallIndex(runtime, "word 20504") <
                  CallIndex(runtime, "destroy name") &&
              CallIndex(runtime, "destroy name") <
                  CallIndex(runtime, "word 20520"),
          "imageset conversion, image lookup and field order");

    Runtime failing;
    failing.throw_image = true;
    bool failed = false;
    try {
        pal4::UiTextImageOverlay::SetImage(
            0x5000u, "Atlas", "Icon", 1u, failing);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && failing.calls.back() == "destroy name" &&
              failing.words.find(0x5018u) == failing.words.end() &&
              failing.words.find(0x5028u) == failing.words.end(),
          "image lookup failure destroys temporary before field writes");
}

void CheckRender() {
    Runtime absent;
    pal4::UiTextImageOverlay::Render(0x5000u, absent);
    Check(absent.calls.size() == 1u &&
              absent.calls.front() == "read word 20500",
          "missing font prevents all render work");

    Runtime transparent;
    transparent.words[0x5014u] = transparent.font;
    transparent.words[0x502cu] = std::bit_cast<std::uint32_t>(0.0f);
    pal4::UiTextImageOverlay::Render(0x5000u, transparent);
    Check(!HasCall(transparent, "render height"),
          "nonpositive alpha prevents render setup");

    Runtime runtime;
    runtime.words[0x5014u] = runtime.font;
    runtime.words[0x502cu] = std::bit_cast<std::uint32_t>(0.5f);
    runtime.words[0x5008u] = 0x8000u;
    runtime.words[0x5024u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x501cu] = std::bit_cast<std::uint32_t>(500.0f);
    runtime.words[0x5020u] = std::bit_cast<std::uint32_t>(300.0f);
    runtime.words[0x5018u] = runtime.image;
    runtime.words[0x5028u] = std::bit_cast<std::uint32_t>(0.5f);
    runtime.words[runtime.image + 28u] =
        std::bit_cast<std::uint32_t>(200.0f);
    runtime.words[runtime.image + 32u] =
        std::bit_cast<std::uint32_t>(100.0f);
    runtime.words[runtime.font + 208u] =
        std::bit_cast<std::uint32_t>(20.0f);
    pal4::UiTextImageOverlay::Render(0x5000u, runtime);
    Check(runtime.image_rect.left == 450.0f &&
              runtime.image_rect.top == 275.0f &&
              runtime.image_rect.right == 550.0f &&
              runtime.image_rect.bottom == 325.0f &&
              runtime.text_rect.left == 460.0f &&
              runtime.text_rect.top == 280.0f &&
              runtime.text_rect.right == 460.0f &&
              runtime.text_rect.bottom == 280.0f,
          "centered image and text geometry");
    Check(runtime.image_clip.right == 1280.0f &&
              runtime.image_clip.bottom == 800.0f &&
              runtime.text_clip.right == 1280.0f &&
              runtime.text_clip.bottom == 800.0f &&
              runtime.image_alpha == 0.5f &&
              runtime.text_alpha == 0.5f &&
              runtime.measured_scale == 2.0f &&
              runtime.drawn_scale == 2.0f,
          "clip, opacity and font scale");
    Check(CallIndex(runtime, "render height") <
              CallIndex(runtime, "render width") &&
              CallIndex(runtime, "convert utf8") <
              CallIndex(runtime, "measure text") &&
              CallIndex(runtime, "measure text") <
              CallIndex(runtime, "draw image") &&
              CallIndex(runtime, "draw image") <
              CallIndex(runtime, "draw text"),
          "measurement, image and text order");
    std::size_t conversions = 0u;
    for (const auto& call : runtime.calls)
        if (call == "convert utf8") ++conversions;
    Check(conversions == 2u, "text converted twice by original draw path");

    Runtime text_only = runtime;
    text_only.calls.clear();
    text_only.words[0x5018u] = 0u;
    pal4::UiTextImageOverlay::Render(0x5000u, text_only);
    Check(!HasCall(text_only, "draw image") &&
              HasCall(text_only, "draw text"),
          "text still draws without an image");
}

void CheckDestroy() {
    for (const std::uint8_t reference : {0u, 0xffu, 2u}) {
        Runtime runtime;
        runtime.words[0x5008u] = 0x8000u;
        runtime.words[0x500cu] = 3u;
        runtime.words[0x5010u] = 31u;
        runtime.bytes[0x7fffu] = reference;
        const auto result =
            pal4::UiTextImageOverlay::Destroy(0x5000u, runtime);
        Check(runtime.words.at(0x5000u) ==
                  pal4::UiTextImageOverlay::kVtable &&
                  runtime.words.at(0x5008u) == 0u &&
                  runtime.words.at(0x500cu) == 0u &&
                  runtime.words.at(0x5010u) == 0u,
              "destructor restores vtable and clears record");
        if (reference == 2u)
            Check(result == 0x7fffu && runtime.bytes.at(0x7fffu) == 1u &&
                      !HasCall(runtime, "free text"),
                  "shared text reference decremented");
        else
            Check(result == 0xdeadu && HasCall(runtime, "free text"),
                  "unique or sentinel text allocation freed");
    }
    Runtime empty;
    Check(pal4::UiTextImageOverlay::Destroy(0x5000u, empty) == 0u &&
              !HasCall(empty, "free text"),
          "empty text requires no free");
}

void CheckAnimation() {
    Runtime runtime;
    Check(pal4::UiTextImageAnimation::Construct(
              0x5000u, 0xa5u, runtime) == 0x5000u &&
              runtime.words.at(0x5000u) ==
                  pal4::UiTextImageAnimation::kVtable &&
              runtime.words.at(0x5040u) == 0u &&
              runtime.words.at(0x5038u) == 0x3f800000u &&
              runtime.words.at(0x503cu) == 0x40000000u,
          "animation base construction and timing defaults");
    Check(pal4::UiTextImageAnimation::SetTiming(
              0x5000u, 0x3f800000u, 0x40400000u, runtime) ==
              0x3f800000u &&
              runtime.words.at(0x503cu) == 0x40400000u,
          "timing setter writes both words and returns first");
    Check(pal4::UiTextImageAnimation::Advance(
              0x5000u, 0.5f, runtime) == 0x100u &&
              runtime.words.at(0x502cu) == 0x3f800000u,
          "progress before start preserves alpha");
    Check(pal4::UiTextImageAnimation::Advance(
              0x5000u, 0.5f, runtime) == 0x4000u &&
              runtime.words.at(0x502cu) == 0x3f800000u,
          "progress at start preserves alpha");
    Check(pal4::UiTextImageAnimation::Advance(
              0x5000u, 1.0f, runtime) == 0u &&
              runtime.words.at(0x502cu) ==
                  std::bit_cast<std::uint32_t>(0.5f) &&
              !pal4::UiTextImageAnimation::IsFinished(0x5000u, runtime),
          "progress after start interpolates alpha");
    (void)pal4::UiTextImageAnimation::Advance(0x5000u, 1.0f, runtime);
    Check(pal4::UiTextImageAnimation::IsFinished(0x5000u, runtime),
          "progress at end is finished");

    runtime.words[0x5040u] =
        std::bit_cast<std::uint32_t>(
            std::numeric_limits<float>::quiet_NaN());
    Check(pal4::UiTextImageAnimation::Advance(
              0x5000u, 1.0f, runtime) == 0x4100u &&
              !pal4::UiTextImageAnimation::IsFinished(0x5000u, runtime),
          "unordered x87 comparison remains unfinished");
}

void CheckDerivedAnimations() {
    Runtime delayed;
    Check(pal4::UiTextImageAnimation::ConstructDelayed(
              0x5000u, 3u, delayed) == 0x5000u &&
              delayed.words.at(0x5000u) ==
                  pal4::UiTextImageAnimation::kDelayedVtable &&
              delayed.words.at(0x5044u) == 0u,
          "delayed animation construction");
    delayed.words[0x5044u] = std::bit_cast<std::uint32_t>(1.0f);
    Check(!pal4::UiTextImageAnimation::AdvanceDelayed(
               0x5000u, 0.5f, delayed).has_value() &&
              delayed.words.at(0x5040u) == 0u &&
              delayed.words.at(0x5044u) ==
                  std::bit_cast<std::uint32_t>(0.5f),
          "positive delay suppresses base advance");
    Check(pal4::UiTextImageAnimation::AdvanceDelayed(
              0x5000u, 1.0f, delayed) == 0x4000u &&
              delayed.words.at(0x5044u) ==
                  std::bit_cast<std::uint32_t>(-0.5f) &&
              delayed.words.at(0x5040u) ==
                  std::bit_cast<std::uint32_t>(1.0f),
          "negative delay invokes base advance with full delta");

    Runtime motion;
    Check(pal4::UiTextImageAnimation::ConstructMotion(
              0x5000u, 4u, motion) == 0x5000u &&
              motion.words.at(0x5000u) ==
                  pal4::UiTextImageAnimation::kMotionVtable &&
              motion.words.at(0x5044u) == 0x3f800000u,
          "motion animation construction");
    motion.words[0x5020u] = std::bit_cast<std::uint32_t>(100.0f);
    Check(pal4::UiTextImageAnimation::AdvanceMotion(
              0x5000u, 1.0f, motion) == 0x4000u &&
              motion.words.at(0x5020u) ==
                  std::bit_cast<std::uint32_t>(99.0f),
          "motion y changes after base animation update");
}

void CheckPulseAnimation() {
    Runtime runtime;
    Check(pal4::UiTextImageAnimation::ConstructPulse(
              0x5000u, 7u, runtime) == 0x5000u &&
              runtime.words.at(0x5000u) ==
                  pal4::UiTextImageAnimation::kPulseVtable,
          "pulse overlay constructor and vtable");
    for (const auto offset : {68u, 72u, 76u, 80u, 84u})
        Check(runtime.words.at(0x5000u + offset) == 0u,
              "pulse state starts clear");

    const auto grow_rate = std::bit_cast<std::uint32_t>(2.0f);
    const auto grow_duration = std::bit_cast<std::uint32_t>(1.0f);
    const auto shrink_rate = std::bit_cast<std::uint32_t>(1.0f);
    const auto shrink_duration = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[0x5054u] = 0xaaaaaaaau;
    Check(pal4::UiTextImageAnimation::SetPulseTiming(
              0x5000u, grow_rate, grow_duration,
              shrink_rate, shrink_duration, runtime) == shrink_rate &&
              runtime.words.at(0x5044u) == grow_rate &&
              runtime.words.at(0x5048u) == shrink_rate &&
              runtime.words.at(0x504cu) == grow_duration &&
              runtime.words.at(0x5050u) == shrink_duration &&
              runtime.words.at(0x5054u) == 0u,
          "pulse setter maps arguments and resets phase timer");

    Check(pal4::UiTextImageAnimation::AdvancePulse(
              0x5000u, 0.5f, runtime) ==
              std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words.at(0x5024u) ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words.at(0x5028u) ==
                  std::bit_cast<std::uint32_t>(2.0f) &&
              runtime.words.at(0x5054u) ==
                  std::bit_cast<std::uint32_t>(0.5f),
          "growing phase synchronizes text and image scale");
    Check(pal4::UiTextImageAnimation::AdvancePulse(
              0x5000u, 0.5f, runtime) ==
              std::bit_cast<std::uint32_t>(3.0f) &&
              runtime.words.at(0x5054u) ==
                  std::bit_cast<std::uint32_t>(1.0f),
          "equal duration remains in growing phase");
    Check(pal4::UiTextImageAnimation::AdvancePulse(
              0x5000u, 0.25f, runtime) == 0u &&
              runtime.words.at(0x5054u) == 0xb8d1b717u &&
              runtime.words.at(0x5028u) ==
                  std::bit_cast<std::uint32_t>(3.5f),
          "duration overrun marks the shrinking phase");
    Check(pal4::UiTextImageAnimation::AdvancePulse(
              0x5000u, 0.25f, runtime) == 0x100u &&
              runtime.words.at(0x5028u) ==
                  std::bit_cast<std::uint32_t>(3.25f),
          "shrinking phase returns x87 less-than mask");
    Check(pal4::UiTextImageAnimation::AdvancePulse(
              0x5000u, 1.0f, runtime) ==
              std::bit_cast<std::uint32_t>(2.25f) &&
              runtime.words.at(0x5054u) == 0u &&
              runtime.words.at(0x5024u) ==
                  std::bit_cast<std::uint32_t>(2.25f),
          "shrinking phase resets after duration overrun");

    runtime.words[0x5054u] =
        std::bit_cast<std::uint32_t>(
            std::numeric_limits<float>::quiet_NaN());
    Check(pal4::UiTextImageAnimation::AdvancePulse(
              0x5000u, 0.25f, runtime) == 0x4100u,
          "unordered phase comparison preserves x87 status mask");
}

}  // namespace

int main() {
    try {
        CheckConstruct(0u, 0x7100u);
        CheckConstruct(0xa5u, 0u);
        CheckFailures();
        CheckTextAndFont();
        CheckTextBranches();
        CheckFontFailure();
        CheckProperties();
        CheckImage();
        CheckRender();
        CheckDestroy();
        CheckAnimation();
        CheckDerivedAnimations();
        CheckPulseAnimation();
        std::cout << "ui_text_image_overlay_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
