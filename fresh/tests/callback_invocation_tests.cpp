#include "pal4/callback_invocation.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeOperations final : public pal4::ICallbackInvocationOperations {
public:
    [[nodiscard]] std::uint32_t CreateObject() override {
        events.emplace_back("create");
        return created_object;
    }

    [[nodiscard]] int InvokeObject(const std::uint32_t object) override {
        events.emplace_back("invoke:" + std::to_string(object));
        return result;
    }

    std::uint32_t created_object = 0x12345678;
    int result = -42;
    std::vector<std::string> events;
};

void ConstructorClearsFieldsStoresTableAndCreatesObject() {
    pal4::CallbackInvocationRecord record{1, 2, 3, nullptr};
    FakeOperations operations;
    Check(&pal4::CallbackInvocation::Initialize(record, operations) == &record,
          "constructor returns the original record");
    Check(record.field_00 == 0 && record.field_04 == 0,
          "constructor clears fields at offsets zero and four");
    Check(record.operations == &operations &&
              record.object == operations.created_object,
          "constructor stores table and its first operation's result");
    Check(operations.events == std::vector<std::string>{"create"},
          "first operation runs once during initialization");
}

void CallUsesSecondOperationAndReturnsItsResult() {
    pal4::CallbackInvocationRecord record{};
    FakeOperations operations;
    static_cast<void>(pal4::CallbackInvocation::Initialize(record, operations));
    Check(pal4::CallbackInvocation::CallVirtualFunction(record) == -42,
          "call returns second operation result");
    Check(operations.events == std::vector<std::string>{
                                   "create", "invoke:305419896"},
          "second operation receives the stored object");
}

void ClearZerosOnlyObservedFieldsAndReturnsInput() {
    pal4::ObservedObjectState::Fields fields{};
    for (std::size_t index = 0; index < fields.size(); ++index) {
        fields[index] = static_cast<std::uint32_t>(index + 1);
    }
    Check(pal4::ObservedObjectState::Clear(fields) == &fields,
          "clear returns original storage");
    Check(fields[0] == 1 && fields[1] == 2 && fields[2] == 3,
          "clear preserves the first three fields");
    for (const auto index : {3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U}) {
        Check(fields[index] == 0, "clear resets each observed field");
    }
}

}  // namespace

int main() {
    ConstructorClearsFieldsStoresTableAndCreatesObject();
    CallUsesSecondOperationAndReturnsItsResult();
    ClearZerosOnlyObservedFieldsAndReturnsInput();
    std::cout << "3 callback-invocation tests passed\n";
}
