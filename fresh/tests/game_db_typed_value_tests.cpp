#include "pal4/game_db_typed_value.h"
#include "pal4/legacy_crc32.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IGameDbTypedValueRuntime {
    std::vector<std::string> events;
    const char* text = "sample";
    std::uint32_t next_object = 0x8000u;
    std::map<std::uint32_t, pal4::GameDbStringObjectState> objects;
    void DestroyString(std::uint32_t object) override {
        events.push_back("destroy:" + std::to_string(object));
    }
    const char* StringData(std::uint32_t object) override {
        events.push_back("read-string:" + std::to_string(object));
        return text;
    }
    std::uint32_t StringLength(std::uint32_t object) override {
        return objects.at(object).length;
    }
    std::uint32_t AllocateStringObject(std::uint32_t bytes) override {
        events.push_back("allocate-object:" + std::to_string(bytes));
        return next_object;
    }
    pal4::GameDbStringObjectState& StringObject(
        std::uint32_t address) override {
        return objects[address];
    }
    std::uint8_t SetErrorFlag(pal4::GameDbStringObjectState& object,
                              std::uint8_t flag) override {
        events.push_back("flag:" + std::to_string(flag));
        object.flag = (object.flag & 0xffffff00u) | flag;
        return flag;
    }
    std::uint32_t AllocateStringText(std::uint32_t length) override {
        events.push_back("allocate-text:" + std::to_string(length));
        return 0xa000u;
    }
    void CopyStringText(std::uint32_t destination, const char* source,
                        std::uint32_t length) override {
        events.push_back("copy-text:" + std::to_string(destination) + ":" +
                         std::string(source, length));
    }
};

struct ErrorRuntime final : pal4::IGameDbTypedValueErrorRuntime {
    std::vector<std::string> events;
    void SetErrorFlag(pal4::GameDbTypedValueErrorState& state,
                      std::uint8_t flag) override {
        events.push_back("flag:" + std::to_string(flag));
        state.flag = flag;
    }
    std::uint32_t AllocateText(std::uint32_t length) override {
        events.push_back("allocate:" + std::to_string(length));
        return 0xa000u;
    }
    void CopyText(std::uint32_t destination, const char* source,
                  std::uint32_t length) override {
        events.push_back("copy:" + std::to_string(destination) + ":" +
                         std::string(source, length));
    }
    void RegisterCleanup() override { events.push_back("register"); }
    void ReleaseText(std::uint32_t data) override {
        events.push_back("release:" + std::to_string(data));
    }
};

struct ValueArrayRuntime final : pal4::IGameDbValueArrayRuntime {
    std::vector<std::string> events;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return 0x7000u;
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

struct ValueArrayLookupRuntime final : pal4::IGameDbValueArrayMutationRuntime {
    Runtime values;
    ErrorRuntime errors;
    std::map<std::uint32_t, pal4::GameDbTypedValueState> entries;
    std::vector<std::uint32_t> accessed;
    std::vector<std::string> allocation_events;
    std::uint32_t next_array = 0x8000u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocation_events.push_back("allocate:" + std::to_string(bytes));
        return next_array;
    }
    void Free(std::uint32_t address) override {
        allocation_events.push_back("free:" + std::to_string(address));
    }
    pal4::GameDbTypedValueState& ValueAt(std::uint32_t address) override {
        accessed.push_back(address);
        return entries[address];
    }
    void DestroyString(std::uint32_t object) override {
        values.DestroyString(object);
    }
    const char* StringData(std::uint32_t object) override {
        return values.StringData(object);
    }
    std::uint32_t StringLength(std::uint32_t object) override {
        return values.StringLength(object);
    }
    std::uint32_t AllocateStringObject(std::uint32_t bytes) override {
        return values.AllocateStringObject(bytes);
    }
    pal4::GameDbStringObjectState& StringObject(
        std::uint32_t address) override {
        return values.StringObject(address);
    }
    std::uint8_t SetErrorFlag(pal4::GameDbStringObjectState& object,
                              std::uint8_t flag) override {
        return values.SetErrorFlag(object, flag);
    }
    std::uint32_t AllocateStringText(std::uint32_t length) override {
        return values.AllocateStringText(length);
    }
    void CopyStringText(std::uint32_t destination, const char* source,
                        std::uint32_t length) override {
        values.CopyStringText(destination, source, length);
    }
    void SetErrorFlag(pal4::GameDbTypedValueErrorState& state,
                      std::uint8_t flag) override {
        errors.SetErrorFlag(state, flag);
    }
    std::uint32_t AllocateText(std::uint32_t length) override {
        return errors.AllocateText(length);
    }
    void CopyText(std::uint32_t destination, const char* source,
                  std::uint32_t length) override {
        errors.CopyText(destination, source, length);
    }
    void RegisterCleanup() override { errors.RegisterCleanup(); }
    void ReleaseText(std::uint32_t data) override {
        errors.ReleaseText(data);
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::GameDbTypedValueState target{2u, 0x7000u};
    pal4::GameDbTypedValueState integer{1u, 42u};
    Check(&pal4::GameDbTypedValue::AssignFrom(
              target, integer, runtime) == &target &&
              target.type == 1u && target.value == 42u &&
              runtime.events == std::vector<std::string>{"destroy:28672"},
          "integer assignment clears old string before setting value");
    Check(pal4::GameDbTypedValue::GetInteger(target) == 42 &&
              pal4::GameDbTypedValue::GetInteger({3u, 42u}) ==
                  static_cast<std::int32_t>(0xcdcdcdcdu),
          "integer getter returns value or native debug sentinel");

    runtime.events.clear();
    pal4::GameDbTypedValueState source_text{2u, 0x9000u};
    Check(&pal4::GameDbTypedValue::Construct(
              target, source_text, runtime) == &target &&
              target.type == 2u && target.value == 0x8000u &&
              runtime.events == std::vector<std::string>{
                  "read-string:36864", "allocate-object:16", "flag:1",
                  "allocate-text:6", "copy-text:40960:sample"},
          "constructor zeroes storage then assigns string without releasing it");
    runtime.events.clear();
    runtime.text = nullptr;
    (void)pal4::GameDbTypedValue::AssignFrom(target, source_text, runtime);
    Check(runtime.events == std::vector<std::string>{
              "destroy:32768", "read-string:36864",
              "allocate-object:16", "flag:1"},
          "null source text uses empty string after prior string cleanup");

    runtime.events.clear();
    pal4::GameDbTypedValueState source_float{
        3u, std::bit_cast<std::uint32_t>(1.25f)};
    (void)pal4::GameDbTypedValue::AssignFrom(target, source_float, runtime);
    Check(target.type == 3u && target.value == source_float.value &&
              runtime.events ==
                  std::vector<std::string>{"destroy:32768"},
          "float assignment preserves IEEE-754 bits and releases old string");
    runtime.events.clear();
    Check(pal4::GameDbTypedValue::SetFloat(target, -2.5f, runtime) ==
              std::bit_cast<std::uint32_t>(-2.5f) &&
              target.type == 3u &&
              pal4::GameDbTypedValue::GetFloat(target) == -2.5f &&
              runtime.events.empty(),
          "float setter writes type three and exact negative float bits");
    runtime.events.clear();
    pal4::GameDbTypedValueState unsupported{4u, 0xffffffffu};
    (void)pal4::GameDbTypedValue::AssignFrom(target, unsupported, runtime);
    Check(target.type == 0u && target.value == 0u &&
              runtime.events.empty(),
          "unknown type clears destination without setter call");
    target = {2u, 0u};
    pal4::GameDbTypedValue::Destroy(target, runtime);
    Check(target.type == 0u && target.value == 0u &&
              runtime.events.empty(),
          "null string object cleanup only clears the tag");
    target = {2u, 0x8000u};
    pal4::GameDbTypedValue::Destroy(target, runtime);
    Check(target.type == 0u && target.value == 0u &&
              runtime.events == std::vector<std::string>{"destroy:32768"},
          "thunk destructor calls string cleanup");
    runtime.events.clear();
    runtime.text = "sample";
    Check(pal4::GameDbTypedValue::SetString(
              target, "abc", runtime) == 0u &&
              target.type == 2u && target.value == 0x8000u &&
              runtime.objects[0x8000u].data == 0xa000u &&
              runtime.objects[0x8000u].length == 3u &&
              runtime.objects[0x8000u].capacity == 3u &&
              runtime.events == std::vector<std::string>{
                  "allocate-object:16", "flag:1",
                  "allocate-text:3", "copy-text:40960:abc"},
          "string setter constructs 16-byte object and copies text");
    runtime.events.clear();
    Check(pal4::GameDbTypedValue::SetString(
              target, "", runtime) == 1u &&
              target.type == 2u && target.value == 0x8000u &&
              runtime.objects[0x8000u].data == 0u &&
              runtime.events == std::vector<std::string>{
                  "destroy:32768", "allocate-object:16", "flag:1"},
          "empty string setter skips character allocation");
    runtime.events.clear();
    runtime.next_object = 0u;
    Check(pal4::GameDbTypedValue::SetString(
              target, "no-memory", runtime) == 0u &&
              target.type == 2u && target.value == 0u &&
              runtime.events == std::vector<std::string>{
                  "destroy:32768", "allocate-object:16"},
          "object allocation failure preserves string tag with null payload");

    runtime.events.clear();
    Check(pal4::GameDbTypedValue::SetInteger(
              target, -17, runtime) == -17 &&
              target.type == 1u &&
              pal4::GameDbTypedValue::GetInteger(target) == -17 &&
              runtime.events.empty(),
          "integer setter stores signed payload without allocation");
    target = {2u, 0x8000u};
    Check(pal4::GameDbTypedValue::SetInteger(
              target, 7, runtime) == 7 &&
              runtime.events == std::vector<std::string>{"destroy:32768"},
          "integer setter cleans a previous string object");
    Check(pal4::GameDbTypedValue::GetFloat(source_float) == 1.25f &&
              pal4::GameDbTypedValue::GetFloat(target) == 0.0f,
          "float getter accepts only type three");

    ErrorRuntime error_runtime;
    pal4::GameDbTypedValueErrorState fallback;
    Check(pal4::GameDbTypedValue::GetStringObject(
              source_text, fallback, 0x55u, error_runtime) == 0x9000u &&
              error_runtime.events.empty(),
          "string object getter returns owned object without static init");
    Check(pal4::GameDbTypedValue::GetStringObject(
              integer, fallback, 0x55u, error_runtime) == 0x93ceccu &&
              fallback.initialized && fallback.flag == 1u &&
              fallback.data == 0xa000u &&
              fallback.length ==
                  std::string("dbField::asString Error!!").size() &&
              error_runtime.events == std::vector<std::string>{
                  "flag:1", "allocate:25",
                  "copy:40960:dbField::asString Error!!", "register"},
          "mismatched string getter creates and registers static error text");
    error_runtime.events.clear();
    Check(pal4::GameDbTypedValue::GetStringObject(
              target, fallback, 0x77u, error_runtime) == 0x93ceccu &&
              error_runtime.events.empty(),
          "fallback string initializes only once");
    pal4::GameDbTypedValue::DeinitializeErrorString(
        fallback, error_runtime);
    Check(fallback.initialized && fallback.data == 0u &&
              fallback.length == 0u && fallback.capacity == 0u &&
              error_runtime.events ==
                  std::vector<std::string>{"release:40960"},
          "static error cleanup releases text but retains once guard");

    runtime.events.clear();
    target = {1u, 15u};
    pal4::GameDbTypedValue::ApplyBounds(target, {0, 10}, runtime);
    Check(pal4::GameDbTypedValue::GetInteger(target) == 10 &&
              runtime.events.empty(),
          "integer above maximum is clamped");
    target = {1u, std::bit_cast<std::uint32_t>(-4)};
    pal4::GameDbTypedValue::ApplyBounds(target, {0, 10}, runtime);
    Check(pal4::GameDbTypedValue::GetInteger(target) == 0,
          "integer below minimum is clamped");
    target = {1u, 7u};
    pal4::GameDbTypedValue::ApplyBounds(target, {0, 10}, runtime);
    Check(target.value == 7u,
          "integer within bounds is preserved");
    pal4::GameDbTypedValue::ApplyBounds(target, {0, 0}, runtime);
    Check(target.value == 7u,
          "zero minimum and maximum disable bounds update");

    target = {3u, std::bit_cast<std::uint32_t>(12.5f)};
    pal4::GameDbTypedValue::ApplyBounds(target, {2, 10}, runtime);
    Check(pal4::GameDbTypedValue::GetFloat(target) == 10.0f,
          "float above integer maximum is clamped to float maximum");
    target.value = std::bit_cast<std::uint32_t>(-1.5f);
    pal4::GameDbTypedValue::ApplyBounds(target, {2, 10}, runtime);
    Check(pal4::GameDbTypedValue::GetFloat(target) == 2.0f,
          "float below integer minimum is clamped to float minimum");

    runtime.next_object = 0x8000u;
    std::string long_text(400u, 'x');
    runtime.text = long_text.c_str();
    runtime.objects[0x8000u].length = 400u;
    target = {2u, 0x8000u};
    runtime.events.clear();
    pal4::GameDbTypedValue::ApplyBounds(target, {0, 300}, runtime);
    Check(target.type == 2u && target.value == 0x8000u &&
              runtime.objects[0x8000u].length == 300u &&
              runtime.events.front() == "read-string:32768" &&
              runtime.events[1u] == "destroy:32768" &&
              runtime.events.back().size() ==
                  std::string("copy-text:40960:").size() + 300u,
          "long string is truncated through owned-object setter");
    runtime.text = "abcde";
    runtime.objects[0x8000u].length = 5u;
    runtime.events.clear();
    pal4::GameDbTypedValue::ApplyBounds(target, {0, 3}, runtime);
    Check(runtime.objects[0x8000u].length == 3u &&
              runtime.events.back() == "copy-text:40960:abc",
          "short string is truncated to configured maximum");
    runtime.events.clear();
    pal4::GameDbTypedValue::ApplyBounds(target, {0, 0}, runtime);
    Check(runtime.events.empty(), "zero bounds leave string untouched");

    ValueArrayRuntime array_runtime;
    pal4::GameDbValueArrayState array_state;
    array_state.flag = 0xaabbcc00u;
    array_state.begin = 0x1234u;
    Check(&pal4::GameDbValueArray::Construct(
              array_state, 0, array_runtime) == &array_state &&
              array_state.flag == 0xaabbcc00u &&
              array_state.begin == 0u && array_state.end == 0u &&
              array_state.capacity == 0u && array_runtime.events.empty(),
          "zero count initializes empty array without allocation");
    Check(&pal4::GameDbValueArray::Construct(
              array_state, 3, array_runtime) == &array_state &&
              array_state.flag == 0xaabbcc03u &&
              array_state.begin == 0x7000u &&
              array_state.end == 0x7000u &&
              array_state.capacity == 0x7018u &&
              array_runtime.events == std::vector<std::string>{
                  "allocate:24", "free:0"},
          "positive count reserves exact eight-byte slots");
    array_runtime.events.clear();
    (void)pal4::GameDbValueArray::Construct(
        array_state, -1, array_runtime);
    Check(array_state.flag == 0xaabbccffu &&
              array_state.begin == 0x7000u &&
              array_state.end == 0x7000u &&
              array_state.capacity == 0x6ff8u &&
              array_runtime.events == std::vector<std::string>{
                  "allocate:0", "free:0"},
          "negative count follows native zero-allocation and wrapped capacity");
    ValueArrayLookupRuntime lookup;
    pal4::GameDbValueArrayState row{0u, 0x7000u, 0x7010u, 0x7020u};
    lookup.entries[0x7000u] = {2u, 0x9000u};
    lookup.entries[0x7008u] = {3u, std::bit_cast<std::uint32_t>(1.25f)};
    pal4::GameDbTypedValueErrorState row_error;
    pal4::GameDbTypedValueErrorState value_error;
    Check(pal4::GameDbValueArray::GetStringObjectAt(
              row, 0u, row_error, value_error, lookup) == 0x9000u &&
              lookup.accessed == std::vector<std::uint32_t>{0x7000u} &&
              lookup.errors.events.empty(),
          "in-range string lookup delegates to typed value");
    Check(pal4::GameDbValueArray::GetStringObjectAt(
              row, 1u, row_error, value_error, lookup) == 0x93ceccu &&
              value_error.initialized && !row_error.initialized,
          "in-range wrong tag creates field fallback");
    lookup.errors.events.clear();
    Check(pal4::GameDbValueArray::GetStringObjectAt(
              row, 2u, row_error, value_error, lookup) == 0x93cee4u &&
              row_error.initialized && row_error.flag == 2u &&
              row_error.length == 11u &&
              lookup.errors.events == std::vector<std::string>{
                  "allocate:11", "copy:40960:dbRow_Error", "register"},
          "out-of-range lookup initializes row fallback once");
    lookup.errors.events.clear();
    Check(pal4::GameDbValueArray::GetStringObjectAt(
              row, 99u, row_error, value_error, lookup) == 0x93cee4u &&
              row_error.flag == 2u && lookup.errors.events.empty(),
          "repeated row fallback retains first initialization");
    pal4::GameDbValueArray::DeinitializeRowError(row_error, lookup);
    Check(row_error.initialized && row_error.data == 0u &&
              row_error.length == 0u && row_error.capacity == 0u &&
              lookup.errors.events ==
                  std::vector<std::string>{"release:40960"},
          "row cleanup releases data while preserving once guard");
    lookup.accessed.clear();
    Check(pal4::GameDbValueArray::GetFloatAt(row, 1u, lookup) == 1.25f &&
              pal4::GameDbValueArray::GetFloatAt(row, 0u, lookup) == 0.0f &&
              pal4::GameDbValueArray::GetFloatAt(row, 2u, lookup) == 0.0f &&
              lookup.accessed ==
                  std::vector<std::uint32_t>{0x7008u, 0x7000u},
          "float lookup checks array bounds and value type");
    Check(pal4::GameDbValueArray::EntryAddressAt(row, -1) == 0x6ff8u &&
              pal4::GameDbValueArray::EntryAddressAt(row, 3) == 0x7018u,
          "unchecked entry arithmetic wraps in native 32-bit width");
    lookup.accessed.clear();
    Check(!pal4::GameDbValueArray::SetAt(
              row, 3u, {1u, 9u}, {0, 5}, lookup) &&
              lookup.accessed.empty(),
          "index greater than count is rejected");
    Check(pal4::GameDbValueArray::SetAt(
              row, 2u, {1u, 9u}, {0, 5}, lookup) &&
              lookup.entries[0x7010u].type == 1u &&
              lookup.entries[0x7010u].value == 5u &&
              lookup.accessed == std::vector<std::uint32_t>{0x7010u},
          "index equal to count follows native assignment and bounds path");
    pal4::LegacyCrc32 crc;
    lookup.entries[0x7000u] = {1u, std::bit_cast<std::uint32_t>(-17)};
    Check(pal4::GameDbValueArray::GetValueAt(row, 0u, lookup, crc) == -17 &&
              !crc.Initialized(),
          "integer conversion returns signed payload without CRC");
    lookup.entries[0x7000u] = {3u, std::bit_cast<std::uint32_t>(-2.75f)};
    Check(pal4::GameDbValueArray::GetValueAt(row, 0u, lookup, crc) == -2,
          "float conversion truncates toward zero");
    lookup.entries[0x7000u] = {4u, 123u};
    Check(pal4::GameDbValueArray::GetValueAt(row, 0u, lookup, crc) == -1 &&
              pal4::GameDbValueArray::GetValueAt(row, 3u, lookup, crc) == -1,
          "unknown tag and out-of-range index return minus one");
    lookup.entries[0x7000u] = {2u, 0x9000u};
    lookup.values.text = "sample";
    lookup.values.events.clear();
    const auto expected_crc = crc.ComputeAndStore(
        reinterpret_cast<const std::uint8_t*>("sample"), 6u);
    Check(pal4::GameDbValueArray::GetValueAt(row, 0u, lookup, crc) ==
              std::bit_cast<std::int32_t>(expected_crc) &&
              lookup.values.events == std::vector<std::string>{
                  "read-string:36864", "allocate-object:16", "flag:1",
                  "allocate-text:6", "copy-text:40960:sample",
                  "read-string:32768", "destroy:32768"},
          "string conversion hashes C text and releases copied temporary");
    ValueArrayLookupRuntime mutation;
    pal4::GameDbValueArrayState growing{};
    Check(pal4::GameDbValueArray::Insert(
              growing, 0u, 1u, {1u, 11u}, mutation) == 0x8008u &&
              growing.begin == 0x8000u && growing.end == 0x8008u &&
              growing.capacity == 0x8008u &&
              mutation.entries[0x8000u].value == 11u &&
              mutation.allocation_events == std::vector<std::string>{
                  "allocate:8", "free:0"},
          "first insertion allocates and constructs one typed slot");
    mutation.next_array = 0x9000u;
    mutation.allocation_events.clear();
    Check(pal4::GameDbValueArray::Insert(
              growing, growing.end, 1u, {1u, 22u}, mutation) == 0x9010u &&
              growing.begin == 0x9000u && growing.end == 0x9010u &&
              growing.capacity == 0x9010u &&
              mutation.entries[0x9000u].value == 11u &&
              mutation.entries[0x9008u].value == 22u &&
              mutation.allocation_events == std::vector<std::string>{
                  "allocate:16", "free:32768"},
          "full array grows by its old element count");
    growing.capacity = 0x9030u;
    mutation.allocation_events.clear();
    Check(pal4::GameDbValueArray::Insert(
              growing, 0x9000u, 1u, {1u, 33u}, mutation) == 8u &&
              growing.end == 0x9018u &&
              mutation.entries[0x9000u].value == 33u &&
              mutation.entries[0x9008u].value == 11u &&
              mutation.entries[0x9010u].value == 22u &&
              mutation.allocation_events.empty(),
          "in-place insertion shifts a longer tail backward");
    Check(pal4::GameDbValueArray::Insert(
              growing, growing.end, 2u, {1u, 44u}, mutation) == 16u &&
              growing.end == 0x9028u &&
              mutation.entries[0x9018u].value == 44u &&
              mutation.entries[0x9020u].value == 44u,
          "in-place insertion constructs a tail shorter than count");
    Check(pal4::GameDbValueArray::Insert(
              growing, growing.end, 0u, {1u, 99u}, mutation) == 0u &&
              growing.end == 0x9028u,
          "zero insertion leaves a spare-capacity array unchanged");
    ValueArrayLookupRuntime short_tail;
    pal4::GameDbValueArrayState middle{0u, 0xb000u, 0xb010u, 0xb030u};
    short_tail.entries[0xb000u] = {1u, 1u};
    short_tail.entries[0xb008u] = {1u, 2u};
    Check(pal4::GameDbValueArray::Insert(
              middle, 0xb008u, 3u, {1u, 9u}, short_tail) == 24u &&
              middle.end == 0xb028u &&
              short_tail.entries[0xb000u].value == 1u &&
              short_tail.entries[0xb008u].value == 9u &&
              short_tail.entries[0xb010u].value == 9u &&
              short_tail.entries[0xb018u].value == 9u &&
              short_tail.entries[0xb020u].value == 2u,
          "in-place insertion with short nonempty tail preserves its value");
    mutation.entries[0x9010u] = {2u, 0x7777u};
    mutation.values.events.clear();
    mutation.allocation_events.clear();
    Check(pal4::GameDbValueArray::Clear(growing, mutation) == 0u &&
              growing.begin == 0u && growing.end == 0u &&
              growing.capacity == 0u &&
              mutation.values.events ==
                  std::vector<std::string>{"destroy:30583"} &&
              mutation.allocation_events ==
                  std::vector<std::string>{"free:36864"},
          "clear destroys elements and releases the array buffer");
    mutation.allocation_events.clear();
    mutation.next_array = 0xa000u;
    pal4::GameDbValueArray::Add(
        growing, {1u, 17u}, {0, 10}, mutation);
    Check(growing.begin == 0xa000u && growing.end == 0xa008u &&
              mutation.entries[0xa000u].value == 10u &&
              mutation.allocation_events == std::vector<std::string>{
                  "allocate:8", "free:0"},
          "append inserts then applies bounds to the last slot");
    mutation.entries[0xd000u] = {1u, 5u};
    mutation.entries[0xd008u] = {1u, 6u};
    mutation.accessed.clear();
    Check(pal4::GameDbValueArray::CopyRange(
              0xd000u, 0xd000u, 0xe000u, mutation) == 0xe000u &&
              mutation.accessed.empty(),
          "empty copy range returns its destination unchanged");
    Check(pal4::GameDbValueArray::CopyRange(
              0xd000u, 0xd010u, 0xe000u, mutation) == 0xe010u &&
              mutation.entries[0xe000u].value == 5u &&
              mutation.entries[0xe008u].value == 6u,
          "range copy constructs both values and returns destination end");
    mutation.accessed.clear();
    Check(pal4::GameDbValueArray::ConstructAt(
              0u, {1u, 7u}, mutation) == 0u &&
              mutation.accessed.empty(),
          "null destination skips typed construction");
    Check(pal4::GameDbValueArray::ConstructAt(
              0xe010u, {1u, 7u}, mutation) == 0xe010u &&
              mutation.entries[0xe010u].value == 7u,
          "nonnull construction delegates to typed value");
    mutation.entries[0xe000u] = {2u, 0x7777u};
    mutation.values.events.clear();
    pal4::GameDbValueArray::DestroyRange(0xe000u, 0xe018u, mutation);
    Check(mutation.values.events ==
              std::vector<std::string>{"destroy:30583"} &&
              mutation.entries[0xe000u].type == 0u &&
              mutation.entries[0xe008u].type == 0u &&
              mutation.entries[0xe010u].type == 0u,
          "range cleanup visits every element and releases owned string");
    ValueArrayLookupRuntime recovered;
    pal4::GameDbValueArrayState recovered_array{0u, 0xf000u,
                                                 0xf010u, 0xf020u};
    recovered.entries[0xf000u] = {1u, 0xffffffefu};
    recovered.entries[0xf008u] = {3u, 0x3f800000u};
    Check(pal4::GameDbValueArray::GetIntegerAt(
              recovered_array, 0u, recovered) == -17 &&
              pal4::GameDbValueArray::GetIntegerAt(
                  recovered_array, 1u, recovered) ==
                  static_cast<std::int32_t>(0xcdcdcdcdu) &&
              pal4::GameDbValueArray::GetIntegerAt(
                  recovered_array, 2u, recovered) == -1 &&
              recovered.accessed == std::vector<std::uint32_t>{
                  0xf000u, 0xf008u},
          "recovered indexed integer getter checks bounds and tag");
    Check(pal4::GameDbValueArray::CleanupProperties(
              recovered_array, recovered) == 0u &&
              recovered_array.begin == 0u &&
              recovered_array.end == 0u &&
              recovered_array.capacity == 0u &&
              recovered.entries[0xf000u].type == 0u &&
              recovered.entries[0xf008u].type == 0u &&
              recovered.allocation_events ==
                  std::vector<std::string>{"free:61440"},
          "recovered property cleanup reuses typed-value range cleanup");
    Check(pal4::GameDbValueArray::GetIntegerAt(
              recovered_array, 0u, recovered) == -1,
          "empty typed-value array returns missing sentinel");
    std::cout << "GameDB typed value tests passed\n";
}
