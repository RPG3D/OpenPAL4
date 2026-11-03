#include "pal4/record_accessors.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void NilStringDataSettersWriteAndReturn() {
    pal4::NilStringDataHeader record{};
    Check(pal4::NilStringDataHeaderAccessors::SetField00(record, 0x10203040) ==
              0x10203040,
          "field-zero setter returns input");
    Check(pal4::NilStringDataHeaderAccessors::SetField04(record, 0x50607080) ==
              0x50607080,
          "field-four setter returns input");
    Check(pal4::NilStringDataHeaderAccessors::SetField08(record, 0x90abcdef) ==
              0x90abcdef,
          "field-eight setter returns input");
    Check(record.field_00 == 0x10203040 && record.field_04 == 0x50607080 &&
              record.field_08 == 0x90abcdef,
          "all three setters update only their own fields");
}

void GetAndClearAt14ReturnsFieldAndClearsOnly0c() {
    pal4::PropertyValueRecord::Fields fields{1, 2, 3, 4, 5, 6, 7};
    Check(pal4::PropertyValueRecord::GetAndClearAt14(fields) == 6,
          "offset-0x14 getter returns field index five");
    Check(fields[3] == 0 && fields[5] == 6 && fields[6] == 7,
          "getter clears index three and preserves returned and adjacent fields");
}

void GetAndClearAt18ReturnsFieldAndClearsOnly0c() {
    pal4::PropertyValueRecord::Fields fields{1, 2, 3, 4, 5, 6, 7};
    Check(pal4::PropertyValueRecord::GetAndClearAt18(fields) == 7,
          "offset-0x18 getter returns field index six");
    Check(fields[3] == 0 && fields[5] == 6 && fields[6] == 7,
          "getter clears index three without changing payload fields");
}

}  // namespace

int main() {
    NilStringDataSettersWriteAndReturn();
    GetAndClearAt14ReturnsFieldAndClearsOnly0c();
    GetAndClearAt18ReturnsFieldAndClearsOnly0c();
    std::cout << "3 record-accessor tests passed\n";
}
