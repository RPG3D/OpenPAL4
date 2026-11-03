#include "pal4/property_access.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyAccessRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::int32_t dispatch_result{-17};
    std::uint32_t dispatch_count{};

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.push_back("float-read:" + std::to_string(address));
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.push_back("float-write:" + std::to_string(address));
        floats[address] = value;
    }
    std::int32_t DispatchByIndex(const pal4::PropertyCollectionState&,
                                 std::uint32_t target,
                                 std::int32_t index,
                                 std::uint32_t slot) override {
        calls.push_back("dispatch:" + std::to_string(target) + ":" +
                        std::to_string(index) + ":" +
                        std::to_string(slot));
        ++dispatch_count;
        return dispatch_result;
    }
};

void PutName(Runtime& runtime, std::uint32_t entry,
             std::uint32_t data, const std::string& name) {
    runtime.words[entry + 4u] = data;
    runtime.words[entry + 8u] =
        static_cast<std::uint32_t>(name.size());
    for (std::size_t i = 0; i < name.size(); ++i)
        runtime.bytes[data + static_cast<std::uint32_t>(i)] =
            static_cast<std::uint8_t>(name[i]);
}

void TestNameLookup() {
    const pal4::PropertyCollectionState collection{0x1234u, 0x4000u, 3};
    Runtime runtime;
    PutName(runtime, 0x4000u, 0x5000u, "red");
    PutName(runtime, 0x4010u, 0x5100u, "blue");
    PutName(runtime, 0x4020u, 0x5200u, "");
    Check(pal4::PropertyAccess::ProcessByName(
              collection, 0x7000u, "red", runtime) == -17 &&
              runtime.calls.back() == "dispatch:28672:0:20",
          "first name dispatches by virtual byte slot 20");
    Check(pal4::PropertyAccess::ProcessByName(
              collection, 0x7000u, "blue", runtime) == -17 &&
              runtime.calls.back() == "dispatch:28672:1:20",
          "later name uses sixteen-byte stride");
    Check(pal4::PropertyAccess::ProcessByName(
              collection, 0x7000u, "", runtime) == -17 &&
              runtime.calls.back() == "dispatch:28672:2:20",
          "zero-length name matches without data reads");
    Check(pal4::PropertyAccess::ProcessByName(
              collection, 0x7000u, "blue\0ignored", runtime) == -17 &&
              runtime.calls.back() == "dispatch:28672:1:20",
          "caller name stops at its first NUL byte");
    const auto dispatch_calls = runtime.calls.size();
    Check(pal4::PropertyAccess::ProcessByName(
              collection, 0x7000u, "blu", runtime) == 3 &&
              pal4::PropertyAccess::ProcessByName(
                  collection, 0x7000u, "blueberry", runtime) == 3 &&
              pal4::PropertyAccess::ProcessByName(
                  collection, 0x7000u, "bluf", runtime) == 3 &&
              runtime.calls.size() > dispatch_calls &&
              runtime.dispatch_count == 4u,
          "short, long and different names return element count");
    Check(pal4::PropertyAccess::ProcessByName(
              {0u, 0u, 0}, 0x7000u, "red", runtime) == 0 &&
              pal4::PropertyAccess::ProcessByName(
                  {0u, 0u, -2}, 0x7000u, "red", runtime) == -2,
          "nonpositive signed count bypasses traversal");
}

void TestScalarAccess() {
    const pal4::PropertyScalarState property{0x1234u, 0x24u, 3, 0x30u};
    Runtime runtime;
    Check(pal4::PropertyAccess::SetFloat(
              property, 0x8000u, -1.25f, runtime) == 0x24u &&
              runtime.floats.at(0x8024u) == -1.25f &&
              pal4::PropertyAccess::GetFloat(property, 0x8000u, runtime) ==
                  -1.25,
          "float setter returns offset and getter widens stored float");
    Check(pal4::PropertyAccess::SetInt(
              property, 0x8000u, 0xfedcba98u, runtime) == 0x24u &&
              pal4::PropertyAccess::GetInt(property, 0x8000u, runtime) ==
                  0xfedcba98u,
          "integer access preserves all 32 bits");
    runtime.words[0x8030u] = 99u;
    Check(pal4::PropertyAccess::SetIndex(
              property, 0x8000u, -1, runtime) == -1 &&
              pal4::PropertyAccess::SetIndex(
                  property, 0x8000u, 3, runtime) == 3 &&
              runtime.words.at(0x8030u) == 99u,
          "negative and count-equal indexes do not write");
    Check(pal4::PropertyAccess::SetIndex(
              property, 0x8000u, 0, runtime) == 0 &&
              pal4::PropertyAccess::SetIndex(
                  property, 0x8000u, 2, runtime) == 2 &&
              pal4::PropertyAccess::GetIndex(
                  property, 0x8000u, runtime) == 2u,
          "valid index bounds use the separate index offset");
}

}  // namespace

int main() {
    TestNameLookup();
    TestScalarAccess();
}
