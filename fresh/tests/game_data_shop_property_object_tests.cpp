#include "pal4/game_data_shop_property_object.h"

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

struct Call {
    std::string name;
    std::uint32_t address = 0;
    std::uint32_t value = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataShopPropertyObjectRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t allocation = 0x9000;
    std::uint32_t base_result = 0x12345678;
    std::uint32_t fail_text = 0;
    std::uint32_t text_calls = 0;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back({"allocate", size});
        return allocation;
    }
    void ConstructBase(std::uint32_t object,
                       std::uint32_t name) override {
        calls.push_back({"construct_base", object, name});
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write_word", address, value});
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"write_byte", address, value});
    }
    void ClearEffectData(std::uint32_t object) override {
        calls.push_back({"clear_effect", object});
    }
    void DestroyText(std::uint32_t address) override {
        calls.push_back({"destroy_text", address});
        if (++text_calls == fail_text)
            throw std::runtime_error("text destroy failed");
    }
    void ProcessElements(std::uint32_t address) override {
        calls.push_back({"process_elements", address});
    }
    void DestroySecondaryRecord(std::uint32_t address) override {
        calls.push_back({"destroy_secondary", address});
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.push_back({"destroy_base", object});
        return base_result;
    }
};

void CheckCreate() {
    Runtime runtime;
    constexpr std::uint32_t name = 0x123456ab;
    Check(pal4::GameDataShopPropertyObject::Create(name, runtime) ==
              0x9000 &&
              runtime.calls.size() == 15 &&
              runtime.calls[0] == Call{"allocate", 140} &&
              runtime.calls[1] ==
                  Call{"construct_base", 0x9000, name} &&
              runtime.calls.back() ==
                  Call{"write_word", 0x9000, 0x8446ac},
          "shop property factory allocation, base and vtable order");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x9000 + offset] == 0xabu,
              "container tag receives low byte of name pointer");
        for (std::uint32_t word = 4; word <= 12; word += 4)
            Check(runtime.words[0x9000 + offset + word] == 0,
                  "container pointer triplet is cleared");
    }
    runtime.calls.clear();
    runtime.allocation = 0;
    Check(pal4::GameDataShopPropertyObject::Create(name, runtime) == 0 &&
              runtime.calls == std::vector<Call>{{"allocate", 140}},
          "allocation failure skips base construction and writes");
}

void CheckDestroy() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x9000;
    runtime.words[object + 96] = 0x5000;
    runtime.words[object + 100] = 0x5000 + 2 * 56;
    runtime.words[object + 112] = 0x7000;
    runtime.words[object + 116] = 0x7000 + 2 * 60;
    runtime.words[object + 128] = 0x8000;
    Check(pal4::GameDataShopPropertyObject::Destroy(object, runtime) ==
              runtime.base_result,
          "destructor forwards Effect base result");
    Check(runtime.calls == std::vector<Call>{
              {"write_word", object, 0x8446ac},
              {"clear_effect", object},
              {"destroy_text", 0x5000 + 36},
              {"destroy_text", 0x5000 + 20},
              {"destroy_text", 0x5000 + 4},
              {"destroy_text", 0x5000 + 56 + 36},
              {"destroy_text", 0x5000 + 56 + 20},
              {"destroy_text", 0x5000 + 56 + 4},
              {"write_word", object + 100, 0x5000},
              {"process_elements", object + 108},
              {"free", 0x8000},
              {"write_word", object + 128, 0},
              {"write_word", object + 132, 0},
              {"write_word", object + 136, 0},
              {"destroy_secondary", 0x7000},
              {"destroy_secondary", 0x7000 + 60},
              {"free", 0x7000},
              {"write_word", object + 112, 0},
              {"write_word", object + 116, 0},
              {"write_word", object + 120, 0},
              {"free", 0x5000},
              {"write_word", object + 96, 0},
              {"write_word", object + 100, 0},
              {"write_word", object + 104, 0},
              {"destroy_base", object}},
          "destructor preserves text, container and base cleanup order");
    Check(runtime.words[object + 96] == 0 &&
              runtime.words[object + 100] == 0 &&
              runtime.words[object + 104] == 0 &&
              runtime.words[object + 112] == 0 &&
              runtime.words[object + 116] == 0 &&
              runtime.words[object + 120] == 0 &&
              runtime.words[object + 128] == 0 &&
              runtime.words[object + 132] == 0 &&
              runtime.words[object + 136] == 0,
          "all three owned pointer triplets are reset");

    Runtime empty;
    Check(pal4::GameDataShopPropertyObject::Destroy(object, empty) ==
              empty.base_result && empty.text_calls == 0,
          "empty record spans skip element destruction");
    Runtime interrupted;
    interrupted.words[object + 96] = 0x5000;
    interrupted.words[object + 100] = 0x5000 + 56;
    interrupted.fail_text = 2;
    try {
        static_cast<void>(pal4::GameDataShopPropertyObject::Destroy(
            object, interrupted));
        Check(false, "text destruction failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text destroy failed" &&
                  interrupted.calls.size() == 4 &&
                  interrupted.words[object + 100] == 0x5000 + 56,
              "failed destruction stops before range reset");
    }
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        std::cout << "game_data_shop_property_object_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
