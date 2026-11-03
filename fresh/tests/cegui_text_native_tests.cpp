#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_game_database.h"

#include <windows.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main(int argc, char** argv) {
    Check(argc == 2, "provide original game root");
    Check(GetACP() == 936u, "original game text requires CP936 ACP");
    pal4::NativeGameDatabase database;
    Check(database.Load(argv[1]), "load original database");

    for (const auto id : {309u, 310u, 311u}) {
        const auto source = database.FindTableTextById("GameString", id);
        Check(source && !source->empty(), "original UI text exists");
        const auto utf8 = pal4::ConvertGameTextToUtf8(*source);
        Check(utf8 && !utf8->empty(), "game converter produces UTF-8");

        const auto wide_size = MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, utf8->data(),
            static_cast<int>(utf8->size()), nullptr, 0);
        Check(wide_size > 0, "converted text is valid UTF-8");
        std::wstring wide(static_cast<std::size_t>(wide_size), L'\0');
        Check(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                  utf8->data(), static_cast<int>(utf8->size()),
                                  wide.data(), wide_size) == wide_size,
              "decode converted text");
        const auto back_size = WideCharToMultiByte(
            CP_ACP, 0, wide.data(), wide_size, nullptr, 0, nullptr, nullptr);
        Check(back_size == source->size(), "ACP roundtrip byte count");
        std::string back(static_cast<std::size_t>(back_size), '\0');
        Check(WideCharToMultiByte(CP_ACP, 0, wide.data(), wide_size,
                                  back.data(), back_size, nullptr, nullptr) ==
                  back_size && back == *source,
              "ACP roundtrip preserves original database text");
    }
    Check(pal4::ConvertGameTextToUtf8("") == std::string{},
          "empty original text is safe");
    std::cout << "PASS: three original UI database strings converted\n";
}
