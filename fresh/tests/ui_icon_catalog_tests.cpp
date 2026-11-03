#include "pal4/ui_icon_catalog.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Names = std::array<std::string_view, 4>;
struct ExpectedCategory {
    std::string_view imageset;
    Names active;
    Names inactive;
};

constexpr std::array<ExpectedCategory, 7> kExpected{{
    {"gy0", {"tou1_2", "tou2_2", "tou4_2", "tou3"},
             {"tou1", "tou2", "tou4", "tou3"}},
    {"gy0", {"zhandoutianhe2", "zhandoulinsha2", "zhandoumengli2",
              "zhandouziying"},
             {"zhandoutianhe", "zhandoulinsha", "zhandoumengli",
              "zhandouziying"}},
    {"gy0", {"tou1bb_2", "tou2bb_2", "tou4bb_2", "tou3bb"},
             {"tou1bb", "tou2bb", "tou4bb", "tou3bb"}},
    {"zhujiemian0", {"touxianganniu1_2", "touxianganniua1_2",
                     "touxianganniuc1_2", "touxianganniud1"},
                    {"touxianganniu1", "touxianganniua1",
                     "touxianganniuc1", "touxianganniud1"}},
    {"zhujiemian0", {"touxianganniu2_2", "touxianganniub2_2",
                     "touxianganniuc2_2", "touxianganniud2"},
                    {"touxianganniu2", "touxianganniub2",
                     "touxianganniuc2", "touxianganniud2"}},
    {"zhujiemian0", {"touxianganniu3_2", "touxianganniub3_2",
                     "touxianganniuc3_2", "touxianganniud3"},
                    {"touxianganniu3", "touxianganniub3",
                     "touxianganniuc3", "touxianganniud3"}},
    {"gy0", {"tou1b_2", "tou2b_2", "tou4b_2", "tou3b"},
             {"tou1b", "tou2b", "tou4b", "tou3b"}},
}};

struct Runtime final : pal4::IUiIconCatalogRuntime {
    std::vector<std::string> calls;
    bool active = false;
    bool fail_imageset = false;
    bool fail_image = false;
    std::string imageset_name;
    std::string image_name;

    void InitializePlayerManager() override { calls.emplace_back("initialize"); }
    bool IsPlayer102Active() override {
        calls.emplace_back("query");
        return active;
    }
    std::uint32_t GetImageset(std::string_view name) override {
        calls.emplace_back("imageset");
        imageset_name = name;
        if (fail_imageset) throw std::runtime_error("imageset failure");
        return 0x12345678u;
    }
    std::uint32_t GetImage(std::uint32_t imageset,
                           std::string_view name) override {
        calls.emplace_back("image");
        Check(imageset == 0x12345678u, "imageset handle forwarding");
        image_name = name;
        if (fail_image) throw std::runtime_error("image failure");
        return 0x87654321u;
    }
};

void CheckCatalog() {
    for (std::int32_t category = 0; category < 7; ++category) {
        for (std::int32_t character = 0; character < 4; ++character) {
            for (std::int32_t state = 0; state < 2; ++state) {
                Runtime runtime;
                runtime.active = state != 0;
                const auto result = pal4::UiIconCatalog::GetIconImage(
                    character, category, 0, runtime);
                Check(result == 0x87654321u, "image return value");
                Check(runtime.calls == std::vector<std::string>{
                    "initialize", "query", "imageset", "image"},
                    "automatic state lookup order");
                const auto& expected = kExpected[category];
                Check(runtime.imageset_name == expected.imageset,
                      "imageset resource name");
                Check(runtime.image_name ==
                          (state != 0 ? expected.active[character]
                                      : expected.inactive[character]),
                      "image resource name");
            }
        }
    }
}

void CheckOverrides() {
    for (const auto override_value : {1, 2, 3, -1}) {
        Runtime runtime;
        runtime.active = override_value != 2;
        Check(pal4::UiIconCatalog::GetIconImage(
                  0, 0, override_value, runtime) == 0x87654321u,
              "override image return value");
        Check(runtime.calls == std::vector<std::string>{"imageset", "image"},
              "explicit state bypasses player manager");
        Check(runtime.image_name ==
                  (override_value == 2 ? "tou1_2" : "tou1"),
              "only override value two selects active image");
    }
}

void CheckInvalidIndices() {
    constexpr std::array<std::int32_t, 4> invalid{
        -1, 7, 100, std::numeric_limits<std::int32_t>::max()};
    for (const auto category : invalid) {
        Runtime runtime;
        Check(pal4::UiIconCatalog::GetIconImage(0, category, 0, runtime) == 0u,
              "invalid category return");
        Check(runtime.calls == std::vector<std::string>{"initialize", "query"},
              "state lookup precedes category validation");
    }
    for (const auto character : invalid) {
        Runtime runtime;
        Check(pal4::UiIconCatalog::GetIconImage(character, 0, 2, runtime) == 0u,
              "invalid character return");
        Check(runtime.calls.empty(), "invalid explicit lookup has no services");
    }
    Runtime negative;
    Check(pal4::UiIconCatalog::GetIconImage(
              std::numeric_limits<std::int32_t>::min(),
              std::numeric_limits<std::int32_t>::min(), 0, negative) == 0u,
          "minimum signed indices return zero");
    Check(negative.calls == std::vector<std::string>{"initialize", "query"},
          "minimum indices still query state");
}

void CheckFailures() {
    for (const bool fail_imageset : {true, false}) {
        Runtime runtime;
        runtime.fail_imageset = fail_imageset;
        runtime.fail_image = !fail_imageset;
        try {
            const auto result = pal4::UiIconCatalog::GetIconImage(
                0, 0, 2, runtime);
            (void)result;
            throw std::runtime_error("missing service exception");
        } catch (const std::runtime_error& error) {
            Check(std::string_view(error.what()) ==
                      (fail_imageset ? "imageset failure" : "image failure"),
                  "service exception forwarded");
        }
        Check(runtime.calls == (fail_imageset
                    ? std::vector<std::string>{"imageset"}
                    : std::vector<std::string>{"imageset", "image"}),
              "lookup stops at failing service");
    }
}

}  // namespace

int main() {
    try {
        CheckCatalog();
        CheckOverrides();
        CheckInvalidIndices();
        CheckFailures();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
