#include "pal4/game_data_shop_goods_object.h"

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
    std::uint32_t address;
    std::uint32_t value = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataShopGoodsObjectRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t allocation = 0x9000;
    std::uint32_t base_result = 0x1234;
    bool fail_text = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back({"allocate", size});
        return allocation;
    }
    void ConstructBase(std::uint32_t object,
                       std::uint32_t name) override {
        calls.push_back({"construct-base", object, name});
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write-word", address, value});
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"write-byte", address, value});
    }
    void ClearEffectData(std::uint32_t object) override {
        calls.push_back({"clear-effect", object});
    }
    void DestroyRecordText(std::uint32_t address) override {
        calls.push_back({"destroy-text", address});
        if (fail_text) throw std::runtime_error("text failure");
    }
    void ProcessElements(std::uint32_t address) override {
        calls.push_back({"process-elements", address});
    }
    void DestroySecondaryRecord(std::uint32_t address) override {
        calls.push_back({"destroy-secondary", address});
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
    std::uint32_t DestroyBase(std::uint32_t object) override {
        calls.push_back({"destroy-base", object});
        return base_result;
    }
};

void CheckCreate() {
    Runtime runtime;
    constexpr std::uint32_t name = 0x123456ab;
    Check(pal4::GameDataShopGoodsObject::Create(name, runtime) == 0x9000 &&
              runtime.calls.size() == 15 &&
              runtime.calls[0] == Call{"allocate", 140} &&
              runtime.calls[1] == Call{"construct-base", 0x9000, name} &&
              runtime.calls.back() == Call{"write-word", 0x9000, 0x8446cc},
          "Good factory call order and vtable");
    for (std::uint32_t offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x9000 + offset] == 0xabu,
              "Good container tag stores low name byte");
        for (std::uint32_t word = 4; word <= 12; word += 4)
            Check(runtime.words[0x9000 + offset + word] == 0,
                  "Good container pointer triplet is empty");
    }
    Runtime failed;
    failed.allocation = 0;
    Check(pal4::GameDataShopGoodsObject::Create(name, failed) == 0 &&
              failed.calls == std::vector<Call>{{"allocate", 140}},
          "allocation failure has no constructor side effects");
}

void CheckDestroy() {
    constexpr std::uint32_t object = 0x9000;
    Runtime runtime;
    runtime.words[object + 96] = 0x5000;
    runtime.words[object + 100] = 0x5000 + 2 * 36;
    runtime.words[object + 112] = 0x7000;
    runtime.words[object + 116] = 0x7000 + 2 * 60;
    runtime.words[object + 128] = 0x8000;
    Check(pal4::GameDataShopGoodsObject::Destroy(object, runtime) ==
              runtime.base_result &&
              runtime.calls == std::vector<Call>{
                  {"write-word", object, 0x8446cc},
                  {"clear-effect", object},
                  {"destroy-text", 0x5000 + 20},
                  {"destroy-text", 0x5000 + 36 + 20},
                  {"write-word", object + 100, 0x5000},
                  {"process-elements", object + 108},
                  {"free", 0x8000},
                  {"write-word", object + 128, 0},
                  {"write-word", object + 132, 0},
                  {"write-word", object + 136, 0},
                  {"destroy-secondary", 0x7000},
                  {"destroy-secondary", 0x7000 + 60},
                  {"free", 0x7000},
                  {"write-word", object + 112, 0},
                  {"write-word", object + 116, 0},
                  {"write-word", object + 120, 0},
                  {"free", 0x5000},
                  {"write-word", object + 96, 0},
                  {"write-word", object + 100, 0},
                  {"write-word", object + 104, 0},
                  {"destroy-base", object}},
          "Good destructor cleans primary, secondary, and base in order");
    Runtime empty;
    Check(pal4::GameDataShopGoodsObject::Destroy(object, empty) ==
              empty.base_result && empty.calls.size() == 17,
          "empty Good object still clears all owned ranges");
    Runtime interrupted;
    interrupted.words[object + 96] = 0x5000;
    interrupted.words[object + 100] = 0x5000 + 36;
    interrupted.fail_text = true;
    try {
        static_cast<void>(pal4::GameDataShopGoodsObject::Destroy(
            object, interrupted));
        Check(false, "text failure should propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failure" &&
                  interrupted.calls.size() == 3 &&
                  interrupted.words[object + 100] == 0x5000 + 36,
              "failure occurs before primary end reset");
    }
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        std::cout << "game_data_shop_goods_object_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
