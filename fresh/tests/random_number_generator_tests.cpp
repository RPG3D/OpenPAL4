#include "pal4/random_number_generator.h"
#include "pal4/comparison_mt_trace.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::RandomNumberGenerator generator;
    Check(generator.Index() == 625u &&
              pal4::RandomNumberGenerator::kStateAddress == 0x8e0a58u &&
              pal4::RandomNumberGenerator::kIndexAddress == 0x8a1780u,
          "IDA global addresses and uninitialized index");
    generator.Seed(0x1105u);
    Check(generator.Index() == 624u &&
              generator.StateAt(0u) == 0x1105u &&
              generator.StateAt(1u) == 69069u * 0x1105u,
          "seed recurrence and last-return value");
    constexpr std::array<std::uint32_t, 10> expected{
        3510405877u, 4290933890u, 2191955339u, 564929546u,
        152112058u, 4262624192u, 2687398418u, 268830360u,
        1763988213u, 578848526u};
    for (const auto value : expected) {
        Check(generator.Next() == value, "first twist and temper outputs");
    }
    std::mt19937 host(0x1105u);
    Check(host() != expected[0],
          "host std::mt19937 uses a different seed recurrence");
    for (std::uint32_t index = 10u; index < 623u; ++index) {
        (void)generator.Next();
    }
    Check(generator.Next() == 730882493u && generator.Index() == 624u &&
              generator.Next() == 2222118351u && generator.Index() == 1u,
          "624-item refresh boundary");
    generator.Seed(0x1105u);
    Check(generator.Index() == 624u && generator.Next() == expected[0],
          "reseeding after draws restores the original sequence");
    pal4::RandomNumberGenerator automatic;
    Check(automatic.Next() == expected[0], "default seed on first draw");
    pal4::RandomNumberGenerator zero;
    zero.Seed(0u);
    Check(zero.Next() == 0u, "zero seed remains zero through twist");
    if(pal4::GetComparisonMtTrace().Enabled()){
        const auto index=generator.Index();
        std::array<std::uint32_t,624> state{};
        for(unsigned n=0;n<state.size();++n)state[n]=generator.StateAt(n);
        std::ostringstream trace;pal4::GetComparisonMtTrace().Write(trace);
        const auto text=trace.str();
        Check(text.find("\"generator\":\"mt\"")!=std::string::npos &&
              text.find("\"state_before\":624,\"state_after\":1")!=std::string::npos &&
              text.find("\"value\":3510405877")!=std::string::npos &&
              text.find("\"owner\":"+std::to_string(reinterpret_cast<std::uintptr_t>(&generator)))!=std::string::npos &&
              text.find("main")!=std::string::npos && text.find("\"source_line\":0")==std::string::npos,
              "MT observer records actual refresh/value/owner and native call location");
        Check(generator.Index()==index,"MT export leaves index untouched");
        for(unsigned n=0;n<state.size();++n)Check(generator.StateAt(n)==state[n],"MT export leaves all624 words untouched");
        Check(generator.Next()==expected[1],"MT export consumes no additional draw");
        std::cout<<"MT observer: golden sequence/refresh/source/owner, export index/all624/next-draw preservation pass\n";
    }
    std::cout << "random number generator tests passed\n";
}
