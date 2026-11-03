#include "pal4/crt_atexit_stub.h"

#include <cstdlib>
#include <array>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeAtexit final : public pal4::ICrtAtexitApi {
public:
    int Register(const pal4::CrtExitCallback callback) override {
        Check(callback != nullptr, "the nullsub_82 callback is non-null");
        if (last_callback != nullptr) {
            Check(callback == last_callback,
                  "repeated registration uses the same callback");
        }
        last_callback = callback;
        ++call_count;
        return return_value;
    }

    pal4::CrtExitCallback last_callback = nullptr;
    int call_count = 0;
    int return_value = 37;
};

void GuardPreservesOtherBitsAndRegistersEveryTime() {
    std::uint8_t guard = 0x80;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::RegisterNullSub(guard, runtime) == 37,
          "first registration returns the CRT result");
    Check(guard == 0x81, "registration sets bit zero and keeps other bits");
    runtime.return_value = -1;
    Check(pal4::CrtExitRegistration::RegisterNullSub(guard, runtime) == -1,
          "later registration propagates its own CRT result");
    Check(runtime.call_count == 2,
          "an already-set guard does not suppress registration");
}

void InitializeCrt3RegistersAtObservedGuard() {
    Check(pal4::CrtExitRegistration::kInitializeCrt3GuardAddress ==
              0x978b3cu,
          "CRT initializer retains the observed global guard address");
    std::uint8_t guard = 0x40u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::InitializeCrt3(guard, runtime) == 37 &&
              guard == 0x41u && runtime.call_count == 1,
          "CRT initializer sets bit zero and registers callback");
    runtime.return_value = 11;
    Check(pal4::CrtExitRegistration::InitializeCrt3(guard, runtime) == 11 &&
              guard == 0x41u && runtime.call_count == 2,
          "already-set guard still registers callback and forwards result");
}

void InitializeCrt4SharesTheGuardAndRepeatsRegistration() {
    std::uint8_t guard = 0x20u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::InitializeCrt4(guard, runtime) == 37 &&
              guard == 0x21u && runtime.call_count == 1,
          "fourth CRT entry sets the shared guard bit");
    Check(pal4::CrtExitRegistration::InitializeCrt3(guard, runtime) == 37 &&
              runtime.call_count == 2,
          "third and fourth entries register the same callback each time");
    Check(pal4::CrtExitRegistration::InitializeCrt5(guard, runtime) == 37 &&
              runtime.call_count == 3 && guard == 0x21u,
          "fifth entry keeps the same guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt6(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt7(guard, runtime) == 37 &&
              runtime.call_count == 5 && guard == 0x21u,
          "sixth and seventh entries register with the same guard");
    {
        std::uint8_t eighth_guard = 0x80u;
        FakeAtexit eighth_runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt8(
                  eighth_guard, eighth_runtime) == 37 &&
                  pal4::CrtExitRegistration::InitializeCrt8(
                      eighth_guard, eighth_runtime) == 37 &&
                  eighth_guard == 0x81u && eighth_runtime.call_count == 2,
              "eighth entry registers on every call and preserves guard bits");
    }
    Check(pal4::CrtExitRegistration::InitializeCrt9(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt10(guard, runtime) == 37 &&
              runtime.call_count == 7 && guard == 0x21u,
          "ninth and tenth entries register the same callback again");
    Check(pal4::CrtExitRegistration::InitializeCrt11(guard, runtime) == 37 &&
              runtime.call_count == 8 && guard == 0x21u,
          "eleventh entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt12(guard, runtime) == 37 &&
              runtime.call_count == 9 && guard == 0x21u,
          "twelfth entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt13(guard, runtime) == 37 &&
              runtime.call_count == 10 && guard == 0x21u,
          "thirteenth entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt14(guard, runtime) == 37 &&
              runtime.call_count == 11 && guard == 0x21u,
          "fourteenth entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt15(guard, runtime) == 37 &&
              runtime.call_count == 12 && guard == 0x21u,
          "fifteenth entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt16(guard, runtime) == 37 &&
              runtime.call_count == 13 && guard == 0x21u,
          "sixteenth entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt17(guard, runtime) == 37 &&
              runtime.call_count == 14 && guard == 0x21u,
          "seventeenth entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt18(guard, runtime) == 37 &&
              runtime.call_count == 15 && guard == 0x21u,
          "eighteenth entry shares guard and registers again");
    Check(pal4::CrtExitRegistration::InitializeCrt19(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt20(guard, runtime) == 37 &&
              runtime.call_count == 17 && guard == 0x21u,
          "nineteenth and twentieth entries repeat shared registration");
    Check(pal4::CrtExitRegistration::InitializeCrt21(guard, runtime) == 37 &&
              runtime.call_count == 18 && guard == 0x21u,
          "twenty-first entry also repeats shared registration");
    Check(pal4::CrtExitRegistration::InitializeCrt22(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt23(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt24(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt25(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt26(guard, runtime) == 37 &&
              runtime.call_count == 23 && guard == 0x21u,
          "twenty-second through twenty-sixth entries repeat registration");
    Check(pal4::CrtExitRegistration::InitializeCrt27(guard, runtime) == 37 &&
              pal4::CrtExitRegistration::InitializeCrt28(guard, runtime) == 37 &&
              runtime.call_count == 25 && guard == 0x21u,
          "twenty-seventh and twenty-eighth entries repeat registration");
    runtime.return_value = -7;
    Check(pal4::CrtExitRegistration::InitializeCrt29(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt29(guard, runtime) == -7 &&
              runtime.call_count == 27 && guard == 0x21u,
          "twenty-ninth entry repeats registration and forwards result");
    Check(pal4::CrtExitRegistration::InitializeCrt30(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt30(guard, runtime) == -7 &&
              runtime.call_count == 29 && guard == 0x21u,
          "thirtieth entry shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt31(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt31(guard, runtime) == -7 &&
              runtime.call_count == 31 && guard == 0x21u,
          "thirty-first entry shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt32(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt32(guard, runtime) == -7 &&
              runtime.call_count == 33 && guard == 0x21u,
          "thirty-second entry shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt33(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt33(guard, runtime) == -7 &&
              runtime.call_count == 35 && guard == 0x21u,
          "thirty-third entry shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt34(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt34(guard, runtime) == -7 &&
              runtime.call_count == 37 && guard == 0x21u,
          "thirty-fourth entry shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt35(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt35(guard, runtime) == -7 &&
              runtime.call_count == 39 && guard == 0x21u,
          "0x437740 shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt36(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt36(guard, runtime) == -7 &&
              runtime.call_count == 41 && guard == 0x21u,
          "0x438ff0 shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt37(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt37(guard, runtime) == -7 &&
              runtime.call_count == 43 && guard == 0x21u,
          "0x439920 shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt38(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt38(guard, runtime) == -7 &&
              runtime.call_count == 45 && guard == 0x21u,
          "0x43c2a0 shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt39(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt39(guard, runtime) == -7 &&
              runtime.call_count == 47 && guard == 0x21u,
          "0x43de30 shares guard and registers on every call");
    Check(pal4::CrtExitRegistration::InitializeCrt40(guard, runtime) == -7 &&
              pal4::CrtExitRegistration::InitializeCrt40(guard, runtime) == -7 &&
              runtime.call_count == 49 && guard == 0x21u,
          "0x43e650 shares guard and registers on every call");
}

void FacetGuardsRegisterInOrderOnEveryCall() {
    Check(pal4::CrtExitRegistration::kFacetGuardOneAddress == 0x978b3cu &&
              pal4::CrtExitRegistration::kFacetGuardTwoAddress == 0x978b30u,
          "facet guard addresses match IDA");
    std::uint8_t first = 0x40u;
    std::uint8_t second = 0x80u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::InitializeFacetGuards(
              first, second, runtime) == 37,
          "second registration result is forwarded");
    Check(first == 0x41u && second == 0x81u && runtime.call_count == 2,
          "both guards are set and both callbacks registered");
    runtime.return_value = -4;
    Check(pal4::CrtExitRegistration::InitializeFacetGuards(
              first, second, runtime) == -4 && runtime.call_count == 4,
          "already set guards still register twice");
    Check(pal4::CrtExitRegistration::RegisterFacetGuardOne(runtime) == -4 &&
              pal4::CrtExitRegistration::RegisterFacetGuardTwo(runtime) == -4 &&
              runtime.call_count == 6,
          "both direct entry points register the same callback");
}

void LaterCallbacksShareFirstGuard() {
    std::uint8_t guard = 0x20u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackOne(
              guard, runtime) == 37 && guard == 0x21u &&
              runtime.call_count == 1,
          "0x418e80 sets the shared guard and registers callback");
    runtime.return_value = -8;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwo(
              guard, runtime) == -8 && guard == 0x21u &&
              runtime.call_count == 2,
          "0x418eb0 repeats registration with already-set guard");
}

void CeguiBufferCallbackUsesSharedGuard() {
    std::uint8_t guard = 0x40u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::RegisterCeguiBufferCallback(
              guard, runtime) == 37 && guard == 0x41u &&
              runtime.call_count == 1,
          "0x4198c0 sets shared bit and registers nullsub_82");
    runtime.return_value = -9;
    Check(pal4::CrtExitRegistration::RegisterCeguiBufferCallback(
              guard, runtime) == -9 && runtime.call_count == 2,
          "repeat call still registers and returns CRT result");
}

void RectangleCallbackUsesSharedGuard() {
    std::uint8_t guard = 0xa0u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::RegisterRectangleCallback(
              guard, runtime) == 37 && guard == 0xa1u &&
              runtime.call_count == 1,
          "0x419e10 registers nullsub_82 and sets shared bit");
    runtime.return_value = -3;
    Check(pal4::CrtExitRegistration::RegisterRectangleCallback(
              guard, runtime) == -3 && runtime.call_count == 2,
          "rectangle callback registers again after guard is set");
}

void FontCallbackUsesSharedGuard() {
    std::uint8_t guard = 0x80u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::RegisterFontCallback(
              guard, runtime) == 37 && guard == 0x81u &&
              runtime.call_count == 1,
          "0x419f10 sets bit zero and registers nullsub_82");
    runtime.return_value = -12;
    Check(pal4::CrtExitRegistration::RegisterFontCallback(
              guard, runtime) == -12 && runtime.call_count == 2,
          "0x419f10 repeats registration on already-set guard");
}

void TextureAndLaterCallbacksUseSharedGuard() {
    std::uint8_t guard = 0x80u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::RegisterTextureDictCallback(
              guard, runtime) == 37 && guard == 0x81u &&
              runtime.call_count == 1,
          "0x41a420 sets shared bit and registers nullsub_82");
    runtime.return_value = -5;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackThree(
              guard, runtime) == -5 && runtime.call_count == 2,
          "0x41a550 registers again with set guard");
    runtime.return_value = -6;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackFour(
              guard, runtime) == -6 && runtime.call_count == 3,
          "0x41a580 registers again and returns CRT result");
    runtime.return_value = -7;
    Check(pal4::CrtExitRegistration::RegisterBlurCallback(
              guard, runtime) == -7 && runtime.call_count == 4,
          "0x41a780 registers blur-adjacent callback with shared guard");
    runtime.return_value = -8;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackFive(
              guard, runtime) == -8 && runtime.call_count == 5,
          "0x41a970 registers again with shared guard");
    runtime.return_value = -9;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackSix(
              guard, runtime) == -9 && runtime.call_count == 6,
          "0x41a9a0 registers again with shared guard");
    runtime.return_value = -10;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackSeven(
              guard, runtime) == -10 && runtime.call_count == 7 &&
              guard == 0x81u,
          "0x41cad0 repeats nullsub_82 registration with shared guard");
    runtime.return_value = -11;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackEight(
              guard, runtime) == -11 && runtime.call_count == 8 &&
              guard == 0x81u,
          "0x41cd00 repeats nullsub_82 registration with shared guard");
    runtime.return_value = -12;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackNine(
              guard, runtime) == -12 && runtime.call_count == 9 &&
              guard == 0x81u,
          "0x41cfc0 repeats nullsub_82 registration with shared guard");
    runtime.return_value = -13;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTen(
              guard, runtime) == -13 && runtime.call_count == 10 &&
              guard == 0x81u,
          "0x41d1b0 repeats nullsub_82 registration with shared guard");
    runtime.return_value = -14;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackEleven(
              guard, runtime) == -14 && runtime.call_count == 11 &&
              guard == 0x81u,
          "0x41d3a0 repeats nullsub_82 registration with shared guard");
    runtime.return_value = -15;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwelve(
              guard, runtime) == -15 && runtime.call_count == 12 &&
              guard == 0x81u,
          "0x41d610 ignores already-set guard and propagates CRT result");
    runtime.return_value = -16;
    Check(pal4::CrtExitRegistration::RegisterLaplaceCallback(
              guard, runtime) == -16 && runtime.call_count == 13 &&
              guard == 0x81u,
          "0x41d800 registers nullsub_82 with the shared guard");
    runtime.return_value = -17;
    Check(pal4::CrtExitRegistration::RegisterOldTVCallback(
              guard, runtime) == -17 && runtime.call_count == 14 &&
              guard == 0x81u,
          "0x41dce0 registers callback despite pre-set guard");
    runtime.return_value = -18;
    Check(pal4::CrtExitRegistration::RegisterPosterizeCallback(
              guard, runtime) == -18 && runtime.call_count == 15 &&
              guard == 0x81u,
          "0x41ded0 registers callback despite pre-set guard");
    runtime.return_value = -19;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackThirteen(
              guard, runtime) == -19 && runtime.call_count == 16 &&
              guard == 0x81u,
          "0x41e0c0 preserves guard and forwards CRT result");
    runtime.return_value = -20;
    Check(pal4::CrtExitRegistration::RegisterRedCallback(
              guard, runtime) == -20 && runtime.call_count == 17 &&
              guard == 0x81u,
          "0x41e150 repeats nullsub_82 registration");
    runtime.return_value = -21;
    Check(pal4::CrtExitRegistration::RegisterSharpenEdgesCallback(
              guard, runtime) == -21 && runtime.call_count == 18 &&
              guard == 0x81u,
          "0x41e340 repeats nullsub_82 registration");
    runtime.return_value = -22;
    Check(pal4::CrtExitRegistration::RegisterTilingCallback(
              guard, runtime) == -22 && runtime.call_count == 19 &&
              guard == 0x81u,
          "0x41e530 repeats nullsub_82 registration");
    runtime.return_value = -23;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackFourteen(
              guard, runtime) == -23 && runtime.call_count == 20 &&
              guard == 0x81u,
          "0x41e780 repeats nullsub_82 registration");
    runtime.return_value = -24;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackFifteen(
              guard, runtime) == -24 && runtime.call_count == 21 &&
              guard == 0x81u,
          "0x41ef30 repeats nullsub_82 registration");
    runtime.return_value = -25;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackSixteen(
              guard, runtime) == -25 && runtime.call_count == 22 &&
              guard == 0x81u,
          "0x41f160 repeats nullsub_82 registration");
    runtime.return_value = -26;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackSeventeen(
              guard, runtime) == -26 && runtime.call_count == 23 &&
              guard == 0x81u,
          "0x41f3e0 repeats nullsub_82 registration");
    runtime.return_value = -27;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackEighteen(
              guard, runtime) == -27 && runtime.call_count == 24 &&
              guard == 0x81u,
          "0x41f700 repeats nullsub_82 registration");
    runtime.return_value = -28;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackNineteen(
              guard, runtime) == -28 && runtime.call_count == 25 &&
              guard == 0x81u,
          "0x41f990 repeats nullsub_82 registration");
    runtime.return_value = -29;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwenty(
              guard, runtime) == -29 && runtime.call_count == 26 &&
              guard == 0x81u,
          "0x421de0 repeats nullsub_82 registration");
    runtime.return_value = -30;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentyOne(
              guard, runtime) == -30 && runtime.call_count == 27 &&
              guard == 0x81u,
          "0x422070 repeats nullsub_82 registration");
    runtime.return_value = -31;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentyTwo(
              guard, runtime) == -31 && runtime.call_count == 28 &&
              guard == 0x81u,
          "0x422470 repeats nullsub_82 registration");
    runtime.return_value = -32;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentyThree(
              guard, runtime) == -32 && runtime.call_count == 29 &&
              guard == 0x81u,
          "0x423560 repeats nullsub_82 registration");
    runtime.return_value = -33;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentyFour(
              guard, runtime) == -33 && runtime.call_count == 30 &&
              guard == 0x81u,
          "0x427480 repeats nullsub_82 registration");
    runtime.return_value = -34;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentyFive(
              guard, runtime) == -34 && runtime.call_count == 31 &&
              guard == 0x81u,
          "0x428ac0 preserves guard and registers nullsub_82");
    runtime.return_value = -35;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentySix(
              guard, runtime) == -35 && runtime.call_count == 32 &&
              guard == 0x81u,
          "0x428b30 registers again despite pre-set guard");
    runtime.return_value = -36;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentySeven(
              guard, runtime) == -36 && runtime.call_count == 33 &&
              guard == 0x81u,
          "0x428ee0 repeats nullsub_82 registration");
    runtime.return_value = -37;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentyEight(
              guard, runtime) == -37 && runtime.call_count == 34 &&
              guard == 0x81u,
          "0x4290e0 repeats nullsub_82 registration");
    runtime.return_value = -38;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackTwentyNine(
              guard, runtime) == -38 && runtime.call_count == 35 &&
              guard == 0x81u,
          "0x429850 repeats nullsub_82 registration");
    runtime.return_value = -39;
    Check(pal4::CrtExitRegistration::RegisterLaterCallbackThirty(
              guard, runtime) == -39 && runtime.call_count == 36 &&
              guard == 0x81u,
          "0x429880 repeats nullsub_82 registration");
}

void SharedGuardCallbackRegistration() {
    std::uint8_t guard = 0x80u;
    FakeAtexit runtime;
    Check(pal4::CrtExitRegistration::RegisterSharedGuardCallback(
              guard, runtime) == 37 && guard == 0x81u &&
              runtime.call_count == 1 && runtime.last_callback != nullptr,
          "sound pool tree CRT entry sets guard and registers callback");
    runtime.return_value = -57;
    Check(pal4::CrtExitRegistration::RegisterSharedGuardCallback(
              guard, runtime) == -57 && guard == 0x81u &&
              runtime.call_count == 2,
          "already-set guard still forwards atexit result");
    std::uint8_t world_guard = 0x80u;
    runtime.return_value = -71;
    Check(pal4::CrtExitRegistration::InitializeGameWorldGuard(
              world_guard, runtime) == -71 && world_guard == 0x81u &&
              runtime.call_count == 3 && runtime.last_callback != nullptr,
          "game world CRT entry sets shared guard and registers callback");
    runtime.return_value = 23;
    Check(pal4::CrtExitRegistration::InitializeGameWorldGuard(
              world_guard, runtime) == 23 && world_guard == 0x81u &&
              runtime.call_count == 4,
          "game world CRT entry registers again when guard is set");
}

}  // namespace

int main() {
    SharedGuardCallbackRegistration();
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::RegisterBehaviorZeroCallback(
                  guard, runtime) == 37 && guard == 0x81u &&
                  runtime.call_count == 1,
              "0x429ac0 uses shared guard and registers nullsub_82");
        runtime.return_value = -40;
        Check(pal4::CrtExitRegistration::RegisterBehaviorFourCallback(
                  guard, runtime) == -40 && runtime.call_count == 2,
              "0x429cd0 registers again with guard already set");
        runtime.return_value = -41;
        Check(pal4::CrtExitRegistration::RegisterBehaviorTwoCallback(
                  guard, runtime) == -41 && runtime.call_count == 3,
              "0x429f20 registers again with guard already set");
        runtime.return_value = -42;
        Check(pal4::CrtExitRegistration::RegisterBehaviorThreeCallback(
                  guard, runtime) == -42 && runtime.call_count == 4,
              "0x42a190 registers again with guard already set");
        runtime.return_value = -43;
        Check(pal4::CrtExitRegistration::RegisterBehaviorPathCallback(
                  guard, runtime) == -43 && runtime.call_count == 5,
              "0x42a3c0 registers again with guard already set");
        runtime.return_value = -44;
        Check(pal4::CrtExitRegistration::RegisterObjectExtendedCallback(
                  guard, runtime) == -44 && runtime.call_count == 6 &&
                  guard == 0x81u,
              "0x42c350 registers again with the shared guard");
        runtime.return_value = -45;
        Check(pal4::CrtExitRegistration::RegisterAfterPathCallback(
                  guard, runtime) == -45 && runtime.call_count == 7 &&
                  guard == 0x81u,
              "0x42c770 registers again with the shared guard");
        runtime.return_value = -46;
        Check(pal4::CrtExitRegistration::RegisterObjectDerivedCallback(
                  guard, runtime) == -46 && runtime.call_count == 8 &&
                  guard == 0x81u,
              "0x42c810 registers again with the shared guard");
        runtime.return_value = -47;
        Check(pal4::CrtExitRegistration::RegisterRandomMovementCallback(
                  guard, runtime) == -47 && runtime.call_count == 9 &&
                  guard == 0x81u,
              "0x42d060 registers again with the shared guard");
        runtime.return_value = -48;
        Check(pal4::CrtExitRegistration::RegisterScriptEventCallback(
                  guard, runtime) == -48 && runtime.call_count == 10 &&
                  guard == 0x81u,
              "0x42dd40 registers again with the shared guard");
        runtime.return_value = -49;
        Check(pal4::CrtExitRegistration::RegisterScriptProcessorCallback(
                  guard, runtime) == -49 && runtime.call_count == 11 &&
                  guard == 0x81u,
              "0x42de30 registers again with the shared guard");
        runtime.return_value = -50;
        Check(pal4::CrtExitRegistration::RegisterScriptEventDataCallback(
                  guard, runtime) == -50 && runtime.call_count == 12 &&
                  guard == 0x81u,
              "0x42e3e0 registers again with the shared guard");
        runtime.return_value = -51;
        Check(pal4::CrtExitRegistration::RegisterConditionArrayCallback(
                  guard, runtime) == -51 && runtime.call_count == 13 &&
                  guard == 0x81u,
              "0x42f240 registers again with the shared guard");
        runtime.return_value = -52;
        Check(pal4::CrtExitRegistration::RegisterCollectionRecordCallback(
                  guard, runtime) == -52 && runtime.call_count == 14 &&
                  guard == 0x81u,
              "0x431270 registers again with the shared guard");
    }
    GuardPreservesOtherBitsAndRegistersEveryTime();
    InitializeCrt3RegistersAtObservedGuard();
    InitializeCrt4SharesTheGuardAndRepeatsRegistration();
    FacetGuardsRegisterInOrderOnEveryCall();
    LaterCallbacksShareFirstGuard();
    CeguiBufferCallbackUsesSharedGuard();
    RectangleCallbackUsesSharedGuard();
    FontCallbackUsesSharedGuard();
    TextureAndLaterCallbacksUseSharedGuard();
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        runtime.return_value = -63;
        Check(pal4::CrtExitRegistration::InitializeCrt41(guard, runtime) ==
                  -63 && guard == 0x81u && runtime.call_count == 1,
              "0x440e20 sets shared guard and registers nullsub_82");
        runtime.return_value = -64;
        Check(pal4::CrtExitRegistration::InitializeCrt41(guard, runtime) ==
                  -64 && guard == 0x81u && runtime.call_count == 2,
              "0x440e20 registers on every call after guard is set");
    }
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        runtime.return_value = -42;
        Check(pal4::CrtExitRegistration::InitializeCrt42(guard, runtime) ==
                  -42 && guard == 0x81u && runtime.call_count == 1,
              "0x442270 sets shared guard and returns CRT registration result");
        runtime.return_value = 19;
        Check(pal4::CrtExitRegistration::InitializeCrt42(guard, runtime) ==
                  19 && guard == 0x81u && runtime.call_count == 2,
              "0x442270 registers again when its guard bit is already set");
    }
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        runtime.return_value = -43;
        Check(pal4::CrtExitRegistration::InitializeCrt43(guard, runtime) ==
                  -43 && guard == 0x81u && runtime.call_count == 1,
              "0x444470 sets the shared guard and registers nullsub_82");
        runtime.return_value = 44;
        Check(pal4::CrtExitRegistration::InitializeCrt44(guard, runtime) ==
                  44 && guard == 0x81u && runtime.call_count == 2,
              "0x4444c0 registers again with the same guard already set");
        runtime.return_value = -45;
        Check(pal4::CrtExitRegistration::InitializeCrt45(guard, runtime) ==
                  -45 && guard == 0x81u && runtime.call_count == 3,
              "0x444560 registers a third time with the shared guard");
        runtime.return_value = 46;
        Check(pal4::CrtExitRegistration::InitializeCrt46(guard, runtime) ==
                  46 && guard == 0x81u && runtime.call_count == 4,
              "0x444a90 registers a fourth time with the shared guard");
        runtime.return_value = -47;
        Check(pal4::CrtExitRegistration::InitializeCrt47(guard, runtime) ==
                  -47 && guard == 0x81u && runtime.call_count == 5,
              "0x445020 registers a fifth time with the shared guard");
        runtime.return_value = 48;
        Check(pal4::CrtExitRegistration::InitializeCrt48(guard, runtime) ==
                  48 && guard == 0x81u && runtime.call_count == 6,
              "0x445310 registers a sixth time with the shared guard");
        runtime.return_value = -49;
        Check(pal4::CrtExitRegistration::InitializeCrt49(guard, runtime) ==
                  -49 && guard == 0x81u && runtime.call_count == 7,
              "0x445eb0 registers a seventh time with the shared guard");
        runtime.return_value = 50;
        Check(pal4::CrtExitRegistration::InitializeCrt50(guard, runtime) ==
                  50 && guard == 0x81u && runtime.call_count == 8,
              "0x447370 registers an eighth time with the shared guard");
        runtime.return_value = 51;
        Check(pal4::CrtExitRegistration::InitializeCrt51(guard, runtime) ==
                  51 && guard == 0x81u && runtime.call_count == 9,
              "0x448300 registers a ninth time with the shared guard");
        runtime.return_value = 52;
        Check(pal4::CrtExitRegistration::InitializeCrt52(guard, runtime) ==
                  52 && guard == 0x81u && runtime.call_count == 10,
              "0x448330 registers a tenth time with the shared guard");
        runtime.return_value = -53;
        Check(pal4::CrtExitRegistration::InitializeCrt53(guard, runtime) ==
                  -53 && guard == 0x81u && runtime.call_count == 11,
              "0x449280 registers an eleventh time with the shared guard");
        runtime.return_value = 54;
        Check(pal4::CrtExitRegistration::InitializeCrt54(guard, runtime) ==
                  54 && guard == 0x81u && runtime.call_count == 12,
              "0x4499e0 registers a twelfth time with the shared guard");
        runtime.return_value = -55;
        Check(pal4::CrtExitRegistration::InitializeCrt55(guard, runtime) ==
                  -55 && guard == 0x81u && runtime.call_count == 13,
              "0x44a200 registers a thirteenth time with the shared guard");
        runtime.return_value = 56;
        Check(pal4::CrtExitRegistration::InitializeCrt56(guard, runtime) ==
                  56 && guard == 0x81u && runtime.call_count == 14,
              "0x44b000 registers a fourteenth time with the shared guard");
        runtime.return_value = -57;
        Check(pal4::CrtExitRegistration::InitializeCrt57(guard, runtime) ==
                  -57 && guard == 0x81u && runtime.call_count == 15,
              "0x44cc10 registers a fifteenth time with the shared guard");
        runtime.return_value = 58;
        Check(pal4::CrtExitRegistration::InitializeCrt58(guard, runtime) ==
                  58 && guard == 0x81u && runtime.call_count == 16,
              "0x44daa0 registers a sixteenth time with the shared guard");
        runtime.return_value = -59;
        Check(pal4::CrtExitRegistration::InitializeCrt59(guard, runtime) ==
                  -59 && guard == 0x81u && runtime.call_count == 17,
              "0x44ed50 registers a seventeenth time with the shared guard");
        runtime.return_value = 60;
        Check(pal4::CrtExitRegistration::InitializeCrt60(guard, runtime) ==
                  60 && guard == 0x81u && runtime.call_count == 18,
              "0x47d160 reuses the shared guard and registers every time");
        runtime.return_value = -61;
        Check(pal4::CrtExitRegistration::InitializeCrt61(guard, runtime) ==
                  -61 && guard == 0x81u && runtime.call_count == 19,
              "0x47dc30 reuses the shared guard and registers every time");
        runtime.return_value = 62;
        Check(pal4::CrtExitRegistration::InitializeCrt62(guard, runtime) ==
                  62 && guard == 0x81u && runtime.call_count == 20,
              "0x482500 reuses the shared guard and registers every time");
        runtime.return_value = -63;
        Check(pal4::CrtExitRegistration::InitializeCrt63(guard, runtime) ==
                  -63 && guard == 0x81u && runtime.call_count == 21,
              "0x483550 reuses the shared guard and registers every time");
        runtime.return_value = 64;
        Check(pal4::CrtExitRegistration::InitializeCrt64(guard, runtime) ==
                  64 && guard == 0x81u && runtime.call_count == 22,
              "0x483b20 reuses the shared guard and registers every time");
        runtime.return_value = -65;
        Check(pal4::CrtExitRegistration::InitializeCrt65(guard, runtime) ==
                  -65 && guard == 0x81u && runtime.call_count == 23,
              "0x485570 reuses the shared guard and registers every time");
        runtime.return_value = 66;
        Check(pal4::CrtExitRegistration::InitializeCrt66(guard, runtime) ==
                  66 && guard == 0x81u && runtime.call_count == 24,
              "0x486df0 reuses the shared guard and registers every time");
        runtime.return_value = -67;
        Check(pal4::CrtExitRegistration::InitializeCrt67(guard, runtime) ==
                  -67 && guard == 0x81u && runtime.call_count == 25,
              "0x48dea0 reuses the shared guard and registers every time");
        runtime.return_value = 68;
        Check(pal4::CrtExitRegistration::InitializeCrt68(guard, runtime) ==
                  68 && guard == 0x81u && runtime.call_count == 26,
              "0x4a8f80 reuses the shared guard and registers every time");
        runtime.return_value = -69;
        Check(pal4::CrtExitRegistration::InitializeCrt69(guard, runtime) ==
                  -69 && guard == 0x81u && runtime.call_count == 27,
              "0x4a8fd0 reuses the shared guard and registers every time");
        runtime.return_value = 70;
        Check(pal4::CrtExitRegistration::InitializeCrt70(guard, runtime) ==
                  70 && guard == 0x81u && runtime.call_count == 28,
              "0x4a9fa0 reuses the shared guard and registers every time");
        runtime.return_value = -71;
        Check(pal4::CrtExitRegistration::InitializeCrt71(guard, runtime) ==
                  -71 && guard == 0x81u && runtime.call_count == 29,
              "0x4a9fd0 reuses the shared guard and registers every time");
        runtime.return_value = 72;
        Check(pal4::CrtExitRegistration::InitializeCrt72(guard, runtime) ==
                  72 && guard == 0x81u && runtime.call_count == 30,
              "0x4aa080 reuses the shared guard and registers every time");
        runtime.return_value = -73;
        Check(pal4::CrtExitRegistration::InitializeCrt73(guard, runtime) ==
                  -73 && guard == 0x81u && runtime.call_count == 31,
              "0x4ab6c0 reuses the shared guard and registers every time");
        runtime.return_value = 74;
        Check(pal4::CrtExitRegistration::InitializeCrt74(guard, runtime) ==
                  74 && guard == 0x81u && runtime.call_count == 32,
              "0x4aeb80 reuses the shared guard and registers every time");
        runtime.return_value = -75;
        Check(pal4::CrtExitRegistration::InitializeCrt75(guard, runtime) ==
                  -75 && guard == 0x81u && runtime.call_count == 33,
              "0x4af730 reuses the shared guard and registers every time");
        runtime.return_value = 76;
        Check(pal4::CrtExitRegistration::InitializeCrt76(guard, runtime) ==
                  76 && guard == 0x81u && runtime.call_count == 34,
              "0x4b34c0 reuses the shared guard and registers every time");
        runtime.return_value = -77;
        Check(pal4::CrtExitRegistration::InitializeCrt77(guard, runtime) ==
                  -77 && guard == 0x81u && runtime.call_count == 35,
              "0x4b3ac0 reuses the shared guard and registers every time");
        runtime.return_value = 78;
        Check(pal4::CrtExitRegistration::InitializeCrt78(guard, runtime) ==
                  78 && guard == 0x81u && runtime.call_count == 36,
              "0x4b5280 reuses the shared guard and registers every time");
        runtime.return_value = -79;
        Check(pal4::CrtExitRegistration::InitializeCrt79(guard, runtime) ==
                  -79 && guard == 0x81u && runtime.call_count == 37,
              "0x4b5510 reuses the shared guard and registers every time");
        runtime.return_value = 80;
        Check(pal4::CrtExitRegistration::InitializeCrt80(guard, runtime) ==
                  80 && guard == 0x81u && runtime.call_count == 38,
              "0x4b6e30 reuses the shared guard and registers every time");
        runtime.return_value = -81;
        Check(pal4::CrtExitRegistration::InitializeCrt81(guard, runtime) ==
                  -81 && guard == 0x81u && runtime.call_count == 39,
              "0x4b9210 reuses the shared guard and registers every time");
        runtime.return_value = 82;
        Check(pal4::CrtExitRegistration::InitializeCrt82(guard, runtime) ==
                  82 && guard == 0x81u && runtime.call_count == 40,
              "0x4bae60 reuses the shared guard and registers every time");
        runtime.return_value = -83;
        Check(pal4::CrtExitRegistration::InitializeCrt83(guard, runtime) ==
                  -83 && guard == 0x81u && runtime.call_count == 41,
              "0x4bae90 reuses the shared guard and registers every time");
        runtime.return_value = 84;
        Check(pal4::CrtExitRegistration::InitializeCrt84(guard, runtime) ==
                  84 && guard == 0x81u && runtime.call_count == 42,
              "0x4bb5f0 reuses the shared guard and registers every time");
        runtime.return_value = -85;
        Check(pal4::CrtExitRegistration::InitializeCrt85(guard, runtime) ==
                  -85 && guard == 0x81u && runtime.call_count == 43,
              "0x4c2240 registers again with the existing guard");
        runtime.return_value = 86;
        Check(pal4::CrtExitRegistration::InitializeCrt85(guard, runtime) ==
                  86 && guard == 0x81u && runtime.call_count == 44,
              "0x4c2240 forwards a second CRT result");
        runtime.return_value = -86;
        Check(pal4::CrtExitRegistration::InitializeCrt86(guard, runtime) ==
                  -86 && guard == 0x81u && runtime.call_count == 45,
              "0x4c2cf0 registers with the existing guard");
        runtime.return_value = 87;
        Check(pal4::CrtExitRegistration::InitializeCrt86(guard, runtime) ==
                  87 && guard == 0x81u && runtime.call_count == 46,
              "0x4c2cf0 forwards every registration result");
        runtime.return_value = -87;
        Check(pal4::CrtExitRegistration::InitializeCrt87(guard, runtime) ==
                  -87 && guard == 0x81u && runtime.call_count == 47,
              "0x4c3310 registers with the existing guard");
        runtime.return_value = 88;
        Check(pal4::CrtExitRegistration::InitializeCrt87(guard, runtime) ==
                  88 && guard == 0x81u && runtime.call_count == 48,
              "0x4c3310 forwards every registration result");
        runtime.return_value = -88;
        Check(pal4::CrtExitRegistration::InitializeCrt88(guard, runtime) ==
                  -88 && guard == 0x81u && runtime.call_count == 49,
              "0x4c4180 registers with the shared guard");
        runtime.return_value = 89;
        Check(pal4::CrtExitRegistration::InitializeCrt89(guard, runtime) ==
                  89 && guard == 0x81u && runtime.call_count == 50,
              "0x4c42b0 registers with the shared guard");
        runtime.return_value = -89;
        Check(pal4::CrtExitRegistration::InitializeCrt89(guard, runtime) ==
                  -89 && guard == 0x81u && runtime.call_count == 51,
              "0x4c42b0 registers on repeated invocation");
        runtime.return_value = 90;
        Check(pal4::CrtExitRegistration::InitializeCrt90(guard, runtime) ==
                  90 && guard == 0x81u && runtime.call_count == 52,
              "0x4c5bc0 registers with the shared guard");
        runtime.return_value = -91;
        Check(pal4::CrtExitRegistration::InitializeCrt91(guard, runtime) ==
                  -91 && guard == 0x81u && runtime.call_count == 53,
              "0x4c5e50 registers with the shared guard");
        runtime.return_value = 92;
        Check(pal4::CrtExitRegistration::InitializeCrt92(guard, runtime) ==
                  92 && guard == 0x81u && runtime.call_count == 54,
              "0x4c6570 registers with the shared guard");
        runtime.return_value = -93;
        Check(pal4::CrtExitRegistration::InitializeCrt93(guard, runtime) ==
                  -93 && guard == 0x81u && runtime.call_count == 55,
              "0x4c65a0 registers with the shared guard");
        runtime.return_value = 94;
        Check(pal4::CrtExitRegistration::InitializeCrt94(guard, runtime) ==
                  94 && guard == 0x81u && runtime.call_count == 56,
              "0x4c6d70 registers with the shared guard");
        runtime.return_value = -95;
        Check(pal4::CrtExitRegistration::InitializeCrt95(guard, runtime) ==
                  -95 && guard == 0x81u && runtime.call_count == 57,
              "0x4c6e40 forwards registration result");
        runtime.return_value = 96;
        Check(pal4::CrtExitRegistration::InitializeCrt96(guard, runtime) ==
                  96 && guard == 0x81u && runtime.call_count == 58,
              "0x4c7ff0 registers with the shared guard");
        runtime.return_value = -97;
        Check(pal4::CrtExitRegistration::InitializeCrt97(guard, runtime) ==
                  -97 && guard == 0x81u && runtime.call_count == 59,
              "0x4c8a20 registers with the shared guard");
        runtime.return_value = 98;
        Check(pal4::CrtExitRegistration::InitializeCrt98(guard, runtime) ==
                  98 && guard == 0x81u && runtime.call_count == 60,
              "0x4caf30 registers with the shared guard");
        runtime.return_value = -99;
        Check(pal4::CrtExitRegistration::InitializeCrt99(guard, runtime) ==
                  -99 && guard == 0x81u && runtime.call_count == 61,
              "0x4cbe90 registers with the shared guard");
        runtime.return_value = 100;
        Check(pal4::CrtExitRegistration::InitializeCrt100(guard, runtime) ==
                  100 && guard == 0x81u && runtime.call_count == 62,
              "0x4ce840 registers with the shared guard");
        runtime.return_value = -100;
        Check(pal4::CrtExitRegistration::InitializeCrt100(guard, runtime) ==
                  -100 && guard == 0x81u && runtime.call_count == 63,
              "0x4ce840 registers again on repeated invocation");
        runtime.return_value = 101;
        Check(pal4::CrtExitRegistration::InitializeCrt101(guard, runtime) ==
                  101 && guard == 0x81u && runtime.call_count == 64,
              "0x4cf310 registers with the shared guard");
        runtime.return_value = -101;
        Check(pal4::CrtExitRegistration::InitializeCrt101(guard, runtime) ==
                  -101 && guard == 0x81u && runtime.call_count == 65,
              "0x4cf310 repeats registration and forwards signed result");
        runtime.return_value = 102;
        Check(pal4::CrtExitRegistration::InitializeCrt102(guard, runtime) ==
                  102 && guard == 0x81u && runtime.call_count == 66,
              "0x4d1ce0 registers with the shared guard");
        runtime.return_value = -102;
        Check(pal4::CrtExitRegistration::InitializeCrt102(guard, runtime) ==
                  -102 && guard == 0x81u && runtime.call_count == 67,
              "0x4d1ce0 registers again and forwards signed result");
        runtime.return_value = 103;
        Check(pal4::CrtExitRegistration::InitializeCrt103(guard, runtime) ==
                  103 && guard == 0x81u && runtime.call_count == 68,
              "0x4d51e0 registers with the shared guard");
        runtime.return_value = -103;
        Check(pal4::CrtExitRegistration::InitializeCrt103(guard, runtime) ==
                  -103 && guard == 0x81u && runtime.call_count == 69,
              "0x4d51e0 repeats registration and forwards signed result");
        runtime.return_value = 104;
        Check(pal4::CrtExitRegistration::InitializeCrt104(guard, runtime) ==
                  104 && guard == 0x81u && runtime.call_count == 70,
              "0x4d8000 registers with the shared guard");
        runtime.return_value = -104;
        Check(pal4::CrtExitRegistration::InitializeCrt104(guard, runtime) ==
                  -104 && guard == 0x81u && runtime.call_count == 71,
              "0x4d8000 repeats registration and forwards signed result");
        runtime.return_value = 105;
        Check(pal4::CrtExitRegistration::InitializeCrt105(guard, runtime) ==
                  105 && guard == 0x81u && runtime.call_count == 72,
              "0x4d89c0 registers with the shared guard");
        runtime.return_value = -105;
        Check(pal4::CrtExitRegistration::InitializeCrt105(guard, runtime) ==
                  -105 && guard == 0x81u && runtime.call_count == 73,
              "0x4d89c0 repeats registration and forwards signed result");
        runtime.return_value = 106;
        Check(pal4::CrtExitRegistration::InitializeCrt106(guard, runtime) ==
                  106 && guard == 0x81u && runtime.call_count == 74,
              "0x4daa30 registers with the shared guard");
        runtime.return_value = -106;
        Check(pal4::CrtExitRegistration::InitializeCrt106(guard, runtime) ==
                  -106 && guard == 0x81u && runtime.call_count == 75,
              "0x4daa30 repeats registration and forwards signed result");
        runtime.return_value = 107;
        Check(pal4::CrtExitRegistration::InitializeCrt107(guard, runtime) ==
                  107 && guard == 0x81u && runtime.call_count == 76,
              "0x4dcfb0 registers with the shared guard");
        runtime.return_value = -107;
        Check(pal4::CrtExitRegistration::InitializeCrt107(guard, runtime) ==
                  -107 && guard == 0x81u && runtime.call_count == 77,
              "0x4dcfb0 repeats registration and forwards signed result");
        runtime.return_value = 108;
        Check(pal4::CrtExitRegistration::InitializeCrt108(guard, runtime) ==
                  108 && guard == 0x81u && runtime.call_count == 78,
              "0x4dea60 registers with the shared guard");
        runtime.return_value = -108;
        Check(pal4::CrtExitRegistration::InitializeCrt108(guard, runtime) ==
                  -108 && guard == 0x81u && runtime.call_count == 79,
              "0x4dea60 repeats registration and forwards signed result");
        runtime.return_value = 109;
        Check(pal4::CrtExitRegistration::InitializeCrt109(guard, runtime) ==
                  109 && guard == 0x81u && runtime.call_count == 80,
              "0x4e0740 registers with the shared guard");
        runtime.return_value = -109;
        Check(pal4::CrtExitRegistration::InitializeCrt109(guard, runtime) ==
                  -109 && guard == 0x81u && runtime.call_count == 81,
              "0x4e0740 repeats registration and forwards signed result");
        runtime.return_value = 110;
        Check(pal4::CrtExitRegistration::InitializeCrt110(guard, runtime) ==
                  110 && guard == 0x81u && runtime.call_count == 82,
              "0x4e2850 registers with the shared guard");
        runtime.return_value = -110;
        Check(pal4::CrtExitRegistration::InitializeCrt110(guard, runtime) ==
                  -110 && guard == 0x81u && runtime.call_count == 83,
              "0x4e2850 repeats registration and forwards signed result");
        runtime.return_value = 111;
        Check(pal4::CrtExitRegistration::InitializeCrt111(guard, runtime) ==
                  111 && guard == 0x81u && runtime.call_count == 84,
              "0x4e44e0 registers with the shared guard");
        runtime.return_value = -111;
        Check(pal4::CrtExitRegistration::InitializeCrt111(guard, runtime) ==
                  -111 && guard == 0x81u && runtime.call_count == 85,
              "0x4e44e0 repeats registration and forwards signed result");
        runtime.return_value = 112;
        Check(pal4::CrtExitRegistration::InitializeCrt112(guard, runtime) ==
                  112 && guard == 0x81u && runtime.call_count == 86,
              "0x4e5560 registers with the shared guard");
        runtime.return_value = -112;
        Check(pal4::CrtExitRegistration::InitializeCrt112(guard, runtime) ==
                  -112 && guard == 0x81u && runtime.call_count == 87,
              "0x4e5560 repeats registration and forwards signed result");
        runtime.return_value = 113;
        Check(pal4::CrtExitRegistration::InitializeCrt113(guard, runtime) ==
                  113 && guard == 0x81u && runtime.call_count == 88,
              "0x4e7270 registers with the shared guard");
        runtime.return_value = -113;
        Check(pal4::CrtExitRegistration::InitializeCrt113(guard, runtime) ==
                  -113 && guard == 0x81u && runtime.call_count == 89,
              "0x4e7270 repeats registration and forwards signed result");
        runtime.return_value = 114;
        Check(pal4::CrtExitRegistration::InitializeCrt114(guard, runtime) ==
                  114 && guard == 0x81u && runtime.call_count == 90,
              "0x4e9900 registers with the shared guard");
        runtime.return_value = -114;
        Check(pal4::CrtExitRegistration::InitializeCrt114(guard, runtime) ==
                  -114 && guard == 0x81u && runtime.call_count == 91,
              "0x4e9900 repeats registration and forwards signed result");
        runtime.return_value = 115;
        Check(pal4::CrtExitRegistration::InitializeCrt115(guard, runtime) ==
                  115 && guard == 0x81u && runtime.call_count == 92,
              "0x4edf20 registers with the shared guard");
        runtime.return_value = -115;
        Check(pal4::CrtExitRegistration::InitializeCrt115(guard, runtime) ==
                  -115 && guard == 0x81u && runtime.call_count == 93,
              "0x4edf20 repeats registration and forwards signed result");
        runtime.return_value = 116;
        Check(pal4::CrtExitRegistration::InitializeCrt116(guard, runtime) ==
                  116 && guard == 0x81u && runtime.call_count == 94,
              "0x4eee60 registers with the shared guard");
        runtime.return_value = -116;
        Check(pal4::CrtExitRegistration::InitializeCrt116(guard, runtime) ==
                  -116 && guard == 0x81u && runtime.call_count == 95,
              "0x4eee60 repeats registration and forwards signed result");
        runtime.return_value = 117;
        Check(pal4::CrtExitRegistration::InitializeCrt117(guard, runtime) ==
                  117 && guard == 0x81u && runtime.call_count == 96,
              "0x4f21f0 registers with the shared guard");
        runtime.return_value = -117;
        Check(pal4::CrtExitRegistration::InitializeCrt117(guard, runtime) ==
                  -117 && guard == 0x81u && runtime.call_count == 97,
              "0x4f21f0 repeats registration and forwards signed result");
        runtime.return_value = 118;
        Check(pal4::CrtExitRegistration::InitializeCrt118(guard, runtime) ==
                  118 && guard == 0x81u && runtime.call_count == 98,
              "0x4f3840 registers with the shared guard");
        runtime.return_value = -118;
        Check(pal4::CrtExitRegistration::InitializeCrt118(guard, runtime) ==
                  -118 && guard == 0x81u && runtime.call_count == 99,
              "0x4f3840 repeats registration and forwards signed result");
        runtime.return_value = 119;
        Check(pal4::CrtExitRegistration::InitializeCrt119(guard, runtime) ==
                  119 && guard == 0x81u && runtime.call_count == 100,
              "0x50b110 registers with the shared guard");
        runtime.return_value = -119;
        Check(pal4::CrtExitRegistration::InitializeCrt119(guard, runtime) ==
                  -119 && guard == 0x81u && runtime.call_count == 101,
              "0x50b110 repeats registration and forwards signed result");
        runtime.return_value = 120;
        Check(pal4::CrtExitRegistration::InitializeCrt120(guard, runtime) ==
                  120 && guard == 0x81u && runtime.call_count == 102,
              "0x50cda0 registers with the shared guard");
        runtime.return_value = -120;
        Check(pal4::CrtExitRegistration::InitializeCrt120(guard, runtime) ==
                  -120 && guard == 0x81u && runtime.call_count == 103,
              "0x50cda0 repeats registration and forwards signed result");
        runtime.return_value = 121;
        Check(pal4::CrtExitRegistration::InitializeCrt121(guard, runtime) ==
                  121 && guard == 0x81u && runtime.call_count == 104,
              "0x50f900 registers with the shared guard");
        runtime.return_value = -121;
        Check(pal4::CrtExitRegistration::InitializeCrt121(guard, runtime) ==
                  -121 && guard == 0x81u && runtime.call_count == 105,
              "0x50f900 repeats registration and forwards signed result");
        runtime.return_value = 122;
        Check(pal4::CrtExitRegistration::InitializeCrt122(guard, runtime) ==
                  122 && guard == 0x81u && runtime.call_count == 106,
              "0x510260 registers with the shared guard");
        runtime.return_value = -122;
        Check(pal4::CrtExitRegistration::InitializeCrt122(guard, runtime) ==
                  -122 && guard == 0x81u && runtime.call_count == 107,
              "0x510260 repeats registration and forwards signed result");
        runtime.return_value = 123;
        Check(pal4::CrtExitRegistration::InitializeCrt123(guard, runtime) ==
                  123 && guard == 0x81u && runtime.call_count == 108,
              "0x514a30 registers with the shared guard");
        runtime.return_value = -123;
        Check(pal4::CrtExitRegistration::InitializeCrt123(guard, runtime) ==
                  -123 && guard == 0x81u && runtime.call_count == 109,
              "0x514a30 repeats registration and forwards signed result");
        runtime.return_value = 124;
        Check(pal4::CrtExitRegistration::InitializeCrt124(guard, runtime) ==
                  124 && guard == 0x81u && runtime.call_count == 110,
              "0x51b730 registers with the shared guard");
        runtime.return_value = -124;
        Check(pal4::CrtExitRegistration::InitializeCrt124(guard, runtime) ==
                  -124 && guard == 0x81u && runtime.call_count == 111,
              "0x51b730 repeats registration and forwards signed result");
        runtime.return_value = 125;
        Check(pal4::CrtExitRegistration::InitializeCrt125(guard, runtime) ==
                  125 && guard == 0x81u && runtime.call_count == 112,
              "0x51c4d0 registers with the shared guard");
        runtime.return_value = -125;
        Check(pal4::CrtExitRegistration::InitializeCrt125(guard, runtime) ==
                  -125 && guard == 0x81u && runtime.call_count == 113,
              "0x51c4d0 repeats registration and forwards signed result");
        runtime.return_value = 126;
        Check(pal4::CrtExitRegistration::InitializeCrt126(guard, runtime) ==
                  126 && guard == 0x81u && runtime.call_count == 114,
              "0x51d4e0 registers with the shared guard");
        runtime.return_value = -126;
        Check(pal4::CrtExitRegistration::InitializeCrt126(guard, runtime) ==
                  -126 && guard == 0x81u && runtime.call_count == 115,
              "0x51d4e0 repeats registration and forwards signed result");
        runtime.return_value = 127;
        Check(pal4::CrtExitRegistration::InitializeCrt127(guard, runtime) ==
                  127 && guard == 0x81u && runtime.call_count == 116,
              "0x51db50 registers with the shared guard");
        runtime.return_value = -127;
        Check(pal4::CrtExitRegistration::InitializeCrt127(guard, runtime) ==
                  -127 && guard == 0x81u && runtime.call_count == 117,
              "0x51db50 repeats registration and forwards signed result");
        runtime.return_value = 128;
        Check(pal4::CrtExitRegistration::InitializeCrt128(guard, runtime) ==
                  128 && guard == 0x81u && runtime.call_count == 118,
              "0x51e4d0 registers with the shared guard");
        runtime.return_value = -128;
        Check(pal4::CrtExitRegistration::InitializeCrt128(guard, runtime) ==
                  -128 && guard == 0x81u && runtime.call_count == 119,
              "0x51e4d0 repeats registration and forwards signed result");
        runtime.return_value = 129;
        Check(pal4::CrtExitRegistration::InitializeCrt129(guard, runtime) ==
                  129 && guard == 0x81u && runtime.call_count == 120,
              "0x51ef60 registers with the shared guard");
        runtime.return_value = -129;
        Check(pal4::CrtExitRegistration::InitializeCrt129(guard, runtime) ==
                  -129 && guard == 0x81u && runtime.call_count == 121,
              "0x51ef60 repeats registration and forwards signed result");
        runtime.return_value = 130;
        Check(pal4::CrtExitRegistration::InitializeCrt130(guard, runtime) ==
                  130 && guard == 0x81u && runtime.call_count == 122,
              "0x520c90 registers with the shared guard");
        runtime.return_value = -130;
        Check(pal4::CrtExitRegistration::InitializeCrt130(guard, runtime) ==
                  -130 && guard == 0x81u && runtime.call_count == 123,
              "0x520c90 repeats registration and forwards signed result");
        runtime.return_value = 131;
        Check(pal4::CrtExitRegistration::InitializeCrt131(guard, runtime) ==
                  131 && guard == 0x81u && runtime.call_count == 124,
              "0x5226b0 registers with the shared guard");
        runtime.return_value = -131;
        Check(pal4::CrtExitRegistration::InitializeCrt131(guard, runtime) ==
                  -131 && guard == 0x81u && runtime.call_count == 125,
              "0x5226b0 repeats registration and forwards signed result");
        runtime.return_value = 132;
        Check(pal4::CrtExitRegistration::InitializeCrt132(guard, runtime) ==
                  132 && guard == 0x81u && runtime.call_count == 126,
              "0x522b90 registers with the shared guard");
        runtime.return_value = -132;
        Check(pal4::CrtExitRegistration::InitializeCrt132(guard, runtime) ==
                  -132 && guard == 0x81u && runtime.call_count == 127,
              "0x522b90 repeats registration and forwards signed result");
        runtime.return_value = 133;
        Check(pal4::CrtExitRegistration::InitializeCrt133(guard, runtime) ==
                  133 && guard == 0x81u && runtime.call_count == 128,
              "0x527980 registers with the shared guard");
        runtime.return_value = -133;
        Check(pal4::CrtExitRegistration::InitializeCrt133(guard, runtime) ==
                  -133 && guard == 0x81u && runtime.call_count == 129,
              "0x527980 repeats registration and forwards signed result");
        runtime.return_value = 134;
        Check(pal4::CrtExitRegistration::InitializeCrt134(guard, runtime) ==
                  134 && guard == 0x81u && runtime.call_count == 130,
              "0x529590 registers with the shared guard");
        runtime.return_value = -134;
        Check(pal4::CrtExitRegistration::InitializeCrt134(guard, runtime) ==
                  -134 && guard == 0x81u && runtime.call_count == 131,
              "0x529590 repeats registration and forwards signed result");
        runtime.return_value = 135;
        Check(pal4::CrtExitRegistration::InitializeCrt135(guard, runtime) ==
                  135 && guard == 0x81u && runtime.call_count == 132,
              "0x529710 registers with the shared guard");
        runtime.return_value = -135;
        Check(pal4::CrtExitRegistration::InitializeCrt135(guard, runtime) ==
                  -135 && guard == 0x81u && runtime.call_count == 133,
              "0x529710 repeats registration and forwards signed result");
        runtime.return_value = 136;
        Check(pal4::CrtExitRegistration::InitializeCrt136(guard, runtime) ==
                  136 && guard == 0x81u && runtime.call_count == 134,
              "0x529740 registers with the shared guard");
        runtime.return_value = -136;
        Check(pal4::CrtExitRegistration::InitializeCrt136(guard, runtime) ==
                  -136 && guard == 0x81u && runtime.call_count == 135,
              "0x529740 repeats registration and forwards signed result");
        runtime.return_value = 137;
        Check(pal4::CrtExitRegistration::InitializeCrt137(guard, runtime) ==
                  137 && guard == 0x81u && runtime.call_count == 136,
              "0x529c40 registers with the shared guard");
        runtime.return_value = -137;
        Check(pal4::CrtExitRegistration::InitializeCrt137(guard, runtime) ==
                  -137 && guard == 0x81u && runtime.call_count == 137,
              "0x529c40 repeats registration and forwards signed result");
        runtime.return_value = 138;
        Check(pal4::CrtExitRegistration::InitializeCrt138(guard, runtime) ==
                  138 && guard == 0x81u && runtime.call_count == 138,
              "0x52aed0 registers with the shared guard");
        runtime.return_value = -138;
        Check(pal4::CrtExitRegistration::InitializeCrt138(guard, runtime) ==
                  -138 && guard == 0x81u && runtime.call_count == 139,
              "0x52aed0 repeats registration and forwards signed result");
        runtime.return_value = 139;
        Check(pal4::CrtExitRegistration::InitializeCrt139(guard, runtime) ==
                  139 && guard == 0x81u && runtime.call_count == 140,
              "0x5300b0 registers with the shared guard");
        runtime.return_value = -139;
        Check(pal4::CrtExitRegistration::InitializeCrt139(guard, runtime) ==
                  -139 && guard == 0x81u && runtime.call_count == 141,
              "0x5300b0 repeats registration and forwards signed result");
        runtime.return_value = 140;
        Check(pal4::CrtExitRegistration::InitializeCrt140(guard, runtime) ==
                  140 && guard == 0x81u && runtime.call_count == 142,
              "0x530890 registers with the shared guard");
        runtime.return_value = -140;
        Check(pal4::CrtExitRegistration::InitializeCrt140(guard, runtime) ==
                  -140 && guard == 0x81u && runtime.call_count == 143,
              "0x530890 repeats registration and forwards signed result");
        runtime.return_value = 141;
        Check(pal4::CrtExitRegistration::InitializeCrt141(guard, runtime) ==
                  141 && guard == 0x81u && runtime.call_count == 144,
              "0x533350 registers with the shared guard");
        runtime.return_value = -141;
        Check(pal4::CrtExitRegistration::InitializeCrt141(guard, runtime) ==
                  -141 && guard == 0x81u && runtime.call_count == 145,
              "0x533350 repeats registration and forwards signed result");
        runtime.return_value = 142;
        Check(pal4::CrtExitRegistration::InitializeCrt142(guard, runtime) ==
                  142 && guard == 0x81u && runtime.call_count == 146,
              "0x533390 registers with the shared guard");
        runtime.return_value = -142;
        Check(pal4::CrtExitRegistration::InitializeCrt142(guard, runtime) ==
                  -142 && guard == 0x81u && runtime.call_count == 147,
              "0x533390 repeats registration and forwards signed result");
        runtime.return_value = 143;
        Check(pal4::CrtExitRegistration::InitializeCrt143(guard, runtime) ==
                  143 && guard == 0x81u && runtime.call_count == 148,
              "0x533f10 registers with the shared guard");
        runtime.return_value = -143;
        Check(pal4::CrtExitRegistration::InitializeCrt143(guard, runtime) ==
                  -143 && guard == 0x81u && runtime.call_count == 149,
              "0x533f10 repeats registration and forwards signed result");
        runtime.return_value = 144;
        Check(pal4::CrtExitRegistration::InitializeCrt144(guard, runtime) ==
                  144 && guard == 0x81u && runtime.call_count == 150,
              "0x53af80 registers with the shared guard");
        runtime.return_value = -144;
        Check(pal4::CrtExitRegistration::InitializeCrt144(guard, runtime) ==
                  -144 && guard == 0x81u && runtime.call_count == 151,
              "0x53af80 repeats registration and forwards signed result");
        runtime.return_value = 145;
        Check(pal4::CrtExitRegistration::InitializeCrt145(guard, runtime) ==
                  145 && guard == 0x81u && runtime.call_count == 152,
              "0x53afc0 registers with the shared guard");
        runtime.return_value = -145;
        Check(pal4::CrtExitRegistration::InitializeCrt145(guard, runtime) ==
                  -145 && guard == 0x81u && runtime.call_count == 153,
              "0x53afc0 repeats registration and forwards signed result");
        runtime.return_value = 146;
        Check(pal4::CrtExitRegistration::InitializeCrt146(guard, runtime) ==
                  146 && guard == 0x81u && runtime.call_count == 154,
              "0x53b850 registers with the shared guard");
        runtime.return_value = -146;
        Check(pal4::CrtExitRegistration::InitializeCrt146(guard, runtime) ==
                  -146 && guard == 0x81u && runtime.call_count == 155,
              "0x53b850 repeats registration and forwards signed result");
        runtime.return_value = 147;
        Check(pal4::CrtExitRegistration::InitializeCrt147(guard, runtime) ==
                  147 && guard == 0x81u && runtime.call_count == 156,
              "0x53c8c0 registers with the shared guard");
        runtime.return_value = -147;
        Check(pal4::CrtExitRegistration::InitializeCrt147(guard, runtime) ==
                  -147 && guard == 0x81u && runtime.call_count == 157,
              "0x53c8c0 repeats registration and forwards signed result");
        runtime.return_value = 148;
        Check(pal4::CrtExitRegistration::InitializeCrt148(guard, runtime) ==
                  148 && guard == 0x81u && runtime.call_count == 158,
              "0x53dcc0 registers with the shared guard");
        runtime.return_value = -148;
        Check(pal4::CrtExitRegistration::InitializeCrt148(guard, runtime) ==
                  -148 && guard == 0x81u && runtime.call_count == 159,
              "0x53dcc0 repeats registration and forwards signed result");
        runtime.return_value = 149;
        Check(pal4::CrtExitRegistration::InitializeCrt149(guard, runtime) ==
                  149 && guard == 0x81u && runtime.call_count == 160,
              "0x53e280 registers with the shared guard");
        runtime.return_value = -149;
        Check(pal4::CrtExitRegistration::InitializeCrt149(guard, runtime) ==
                  -149 && guard == 0x81u && runtime.call_count == 161,
              "0x53e280 repeats registration and forwards signed result");
        runtime.return_value = 150;
        Check(pal4::CrtExitRegistration::InitializeCrt150(guard, runtime) ==
                  150 && guard == 0x81u && runtime.call_count == 162,
              "0x53f0e0 registers with the shared guard");
        runtime.return_value = -150;
        Check(pal4::CrtExitRegistration::InitializeCrt150(guard, runtime) ==
                  -150 && guard == 0x81u && runtime.call_count == 163,
              "0x53f0e0 repeats registration and forwards signed result");
        runtime.return_value = 151;
        Check(pal4::CrtExitRegistration::InitializeCrt151(guard, runtime) ==
                  151 && guard == 0x81u && runtime.call_count == 164,
              "0x53fab0 registers with the shared guard");
        runtime.return_value = -151;
        Check(pal4::CrtExitRegistration::InitializeCrt151(guard, runtime) ==
                  -151 && guard == 0x81u && runtime.call_count == 165,
              "0x53fab0 repeats registration and forwards signed result");
        runtime.return_value = 152;
        Check(pal4::CrtExitRegistration::InitializeCrt152(guard, runtime) ==
                  152 && guard == 0x81u && runtime.call_count == 166,
              "0x53fae0 registers with the shared guard");
        runtime.return_value = -152;
        Check(pal4::CrtExitRegistration::InitializeCrt152(guard, runtime) ==
                  -152 && guard == 0x81u && runtime.call_count == 167,
              "0x53fae0 repeats registration and forwards signed result");
        runtime.return_value = 153;
        Check(pal4::CrtExitRegistration::InitializeCrt153(guard, runtime) ==
                  153 && guard == 0x81u && runtime.call_count == 168,
              "0x540f20 registers with the shared guard");
        runtime.return_value = -153;
        Check(pal4::CrtExitRegistration::InitializeCrt153(guard, runtime) ==
                  -153 && guard == 0x81u && runtime.call_count == 169,
              "0x540f20 repeats registration and forwards signed result");
        runtime.return_value = 154;
        Check(pal4::CrtExitRegistration::InitializeCrt154(guard, runtime) ==
                  154 && guard == 0x81u && runtime.call_count == 170,
              "0x542b20 registers with the shared guard");
        runtime.return_value = -154;
        Check(pal4::CrtExitRegistration::InitializeCrt154(guard, runtime) ==
                  -154 && guard == 0x81u && runtime.call_count == 171,
              "0x542b20 repeats registration and forwards signed result");
        runtime.return_value = 155;
        Check(pal4::CrtExitRegistration::InitializeCrt155(guard, runtime) ==
                  155 && guard == 0x81u && runtime.call_count == 172,
              "0x542cb0 registers with the shared guard");
        runtime.return_value = -155;
        Check(pal4::CrtExitRegistration::InitializeCrt155(guard, runtime) ==
                  -155 && guard == 0x81u && runtime.call_count == 173,
              "0x542cb0 repeats registration and forwards signed result");
        runtime.return_value = 156;
        Check(pal4::CrtExitRegistration::InitializeCrt156(guard, runtime) ==
                  156 && guard == 0x81u && runtime.call_count == 174,
              "0x549e30 registers with the shared guard");
        runtime.return_value = -156;
        Check(pal4::CrtExitRegistration::InitializeCrt156(guard, runtime) ==
                  -156 && guard == 0x81u && runtime.call_count == 175,
              "0x549e30 repeats registration and forwards signed result");
        runtime.return_value = 157;
        Check(pal4::CrtExitRegistration::InitializeCrt157(guard, runtime) ==
                  157 && guard == 0x81u && runtime.call_count == 176,
              "0x54afd0 registers with the shared guard");
        runtime.return_value = -157;
        Check(pal4::CrtExitRegistration::InitializeCrt157(guard, runtime) ==
                  -157 && guard == 0x81u && runtime.call_count == 177,
              "0x54afd0 repeats registration and forwards signed result");
        runtime.return_value = 158;
        Check(pal4::CrtExitRegistration::InitializeCrt158(guard, runtime) ==
                  158 && guard == 0x81u && runtime.call_count == 178,
              "0x54e640 registers with the shared guard");
        runtime.return_value = -158;
        Check(pal4::CrtExitRegistration::InitializeCrt158(guard, runtime) ==
                  -158 && guard == 0x81u && runtime.call_count == 179,
              "0x54e640 repeats registration and forwards signed result");
        runtime.return_value = 159;
        Check(pal4::CrtExitRegistration::InitializeCrt159(guard, runtime) ==
                  159 && guard == 0x81u && runtime.call_count == 180,
              "0x54f250 registers with the shared guard");
        runtime.return_value = -159;
        Check(pal4::CrtExitRegistration::InitializeCrt159(guard, runtime) ==
                  -159 && guard == 0x81u && runtime.call_count == 181,
              "0x54f250 repeats registration and forwards signed result");
        runtime.return_value = 160;
        Check(pal4::CrtExitRegistration::InitializeCrt160(guard, runtime) ==
                  160 && guard == 0x81u && runtime.call_count == 182,
              "0x54f280 registers with the shared guard");
        runtime.return_value = -160;
        Check(pal4::CrtExitRegistration::InitializeCrt160(guard, runtime) ==
                  -160 && guard == 0x81u && runtime.call_count == 183,
              "0x54f280 repeats registration and forwards signed result");
        runtime.return_value = 161;
        Check(pal4::CrtExitRegistration::InitializeCrt161(guard, runtime) ==
                  161 && guard == 0x81u && runtime.call_count == 184,
              "0x552160 registers with guard byte_978B3C");
        runtime.return_value = -161;
        Check(pal4::CrtExitRegistration::InitializeCrt161(guard, runtime) ==
                  -161 && guard == 0x81u && runtime.call_count == 185,
              "0x552160 repeats registration and forwards signed result");
        runtime.return_value = 162;
        Check(pal4::CrtExitRegistration::InitializeCrt162(guard, runtime) ==
                  162 && guard == 0x81u && runtime.call_count == 186,
              "0x5550d0 registers nullsub_82 with shared guard");
        runtime.return_value = -162;
        Check(pal4::CrtExitRegistration::InitializeCrt162(guard, runtime) ==
                  -162 && guard == 0x81u && runtime.call_count == 187,
              "0x5550d0 repeats registration and forwards signed result");
        runtime.return_value = 163;
        Check(pal4::CrtExitRegistration::InitializeCrt163(guard, runtime) ==
                  163 && guard == 0x81u && runtime.call_count == 188,
              "0x555f90 registers nullsub_82 with shared guard");
        runtime.return_value = -163;
        Check(pal4::CrtExitRegistration::InitializeCrt163(guard, runtime) ==
                  -163 && guard == 0x81u && runtime.call_count == 189,
              "0x555f90 repeats registration and forwards signed result");
        runtime.return_value = 164;
        Check(pal4::CrtExitRegistration::InitializeCrt164(guard, runtime) ==
                  164 && guard == 0x81u && runtime.call_count == 190,
              "0x556140 registers nullsub_82 with shared guard");
        runtime.return_value = -164;
        Check(pal4::CrtExitRegistration::InitializeCrt164(guard, runtime) ==
                  -164 && guard == 0x81u && runtime.call_count == 191,
              "0x556140 repeats registration and forwards signed result");
        runtime.return_value = 165;
        Check(pal4::CrtExitRegistration::InitializeCrt165(guard, runtime) ==
                  165 && guard == 0x81u && runtime.call_count == 192,
              "0x557310 registers nullsub_82 with shared guard");
        runtime.return_value = -165;
        Check(pal4::CrtExitRegistration::InitializeCrt165(guard, runtime) ==
                  -165 && guard == 0x81u && runtime.call_count == 193,
              "0x557310 repeats registration and forwards signed result");
        runtime.return_value = 166;
        Check(pal4::CrtExitRegistration::InitializeCrt166(guard, runtime) ==
                  166 && guard == 0x81u && runtime.call_count == 194,
              "0x5577a0 registers nullsub_82 with shared guard");
        runtime.return_value = -166;
        Check(pal4::CrtExitRegistration::InitializeCrt166(guard, runtime) ==
                  -166 && guard == 0x81u && runtime.call_count == 195,
              "0x5577a0 repeats registration and forwards signed result");
        runtime.return_value = 167;
        Check(pal4::CrtExitRegistration::InitializeCrt167(guard, runtime) ==
                  167 && guard == 0x81u && runtime.call_count == 196,
              "0x5584e0 registers nullsub_82 with shared guard");
        runtime.return_value = -167;
        Check(pal4::CrtExitRegistration::InitializeCrt167(guard, runtime) ==
                  -167 && guard == 0x81u && runtime.call_count == 197,
              "0x5584e0 repeats registration and forwards signed result");
        runtime.return_value = 168;
        Check(pal4::CrtExitRegistration::InitializeCrt168(guard, runtime) ==
                  168 && guard == 0x81u && runtime.call_count == 198,
              "0x558ca0 registers nullsub_82 with shared guard");
        runtime.return_value = -168;
        Check(pal4::CrtExitRegistration::InitializeCrt168(guard, runtime) ==
                  -168 && guard == 0x81u && runtime.call_count == 199,
              "0x558ca0 repeats registration and forwards signed result");
        runtime.return_value = 169;
        Check(pal4::CrtExitRegistration::InitializeCrt169(guard, runtime) ==
                  169 && guard == 0x81u && runtime.call_count == 200,
              "0x55a320 registers nullsub_82 with shared guard");
        runtime.return_value = -169;
        Check(pal4::CrtExitRegistration::InitializeCrt169(guard, runtime) ==
                  -169 && guard == 0x81u && runtime.call_count == 201,
              "0x55a320 repeats registration and forwards signed result");
        runtime.return_value = 170;
        Check(pal4::CrtExitRegistration::InitializeCrt170(guard, runtime) ==
                  170 && guard == 0x81u && runtime.call_count == 202,
              "0x55b7f0 registers nullsub_82 with shared guard");
        runtime.return_value = -170;
        Check(pal4::CrtExitRegistration::InitializeCrt170(guard, runtime) ==
                  -170 && guard == 0x81u && runtime.call_count == 203,
              "0x55b7f0 repeats registration and forwards signed result");
        runtime.return_value = 171;
        Check(pal4::CrtExitRegistration::InitializeCrt171(guard, runtime) ==
                  171 && guard == 0x81u && runtime.call_count == 204,
              "0x55ce50 registers nullsub_82 with shared guard");
        runtime.return_value = -171;
        Check(pal4::CrtExitRegistration::InitializeCrt171(guard, runtime) ==
                  -171 && guard == 0x81u && runtime.call_count == 205,
              "0x55ce50 repeats registration and forwards signed result");
        runtime.return_value = 172;
        Check(pal4::CrtExitRegistration::InitializeCrt172(guard, runtime) ==
                  172 && guard == 0x81u && runtime.call_count == 206,
              "0x55f730 registers nullsub_82 with shared guard");
        runtime.return_value = -172;
        Check(pal4::CrtExitRegistration::InitializeCrt172(guard, runtime) ==
                  -172 && guard == 0x81u && runtime.call_count == 207,
              "0x55f730 repeats registration and forwards signed result");
        runtime.return_value = 173;
        Check(pal4::CrtExitRegistration::InitializeCrt173(guard, runtime) ==
                  173 && guard == 0x81u && runtime.call_count == 208,
              "0x562520 registers nullsub_82 with shared guard");
        runtime.return_value = -173;
        Check(pal4::CrtExitRegistration::InitializeCrt173(guard, runtime) ==
                  -173 && guard == 0x81u && runtime.call_count == 209,
              "0x562520 repeats registration and forwards signed result");
        runtime.return_value = 174;
        Check(pal4::CrtExitRegistration::InitializeCrt174(guard, runtime) ==
                  174 && guard == 0x81u && runtime.call_count == 210,
              "0x562550 registers nullsub_82 with shared guard");
        runtime.return_value = -174;
        Check(pal4::CrtExitRegistration::InitializeCrt174(guard, runtime) ==
                  -174 && guard == 0x81u && runtime.call_count == 211,
              "0x562550 repeats registration and forwards signed result");
        runtime.return_value = 175;
        Check(pal4::CrtExitRegistration::InitializeCrt175(guard, runtime) ==
                  175 && guard == 0x81u && runtime.call_count == 212,
              "0x5628f0 registers nullsub_82 with shared guard");
        runtime.return_value = -175;
        Check(pal4::CrtExitRegistration::InitializeCrt175(guard, runtime) ==
                  -175 && guard == 0x81u && runtime.call_count == 213,
              "0x5628f0 repeats registration and forwards signed result");
        runtime.return_value = 176;
        Check(pal4::CrtExitRegistration::InitializeCrt176(guard, runtime) ==
                  176 && guard == 0x81u && runtime.call_count == 214,
              "0x562d40 registers nullsub_82 with shared guard");
        runtime.return_value = -176;
        Check(pal4::CrtExitRegistration::InitializeCrt176(guard, runtime) ==
                  -176 && guard == 0x81u && runtime.call_count == 215,
              "0x562d40 repeats registration and forwards signed result");
        runtime.return_value = 177;
        Check(pal4::CrtExitRegistration::InitializeCrt177(guard, runtime) ==
                  177 && guard == 0x81u && runtime.call_count == 216,
              "0x562da0 registers nullsub_82 with shared guard");
        runtime.return_value = -177;
        Check(pal4::CrtExitRegistration::InitializeCrt177(guard, runtime) ==
                  -177 && guard == 0x81u && runtime.call_count == 217,
              "0x562da0 repeats registration and forwards signed result");
        runtime.return_value = 178;
        Check(pal4::CrtExitRegistration::InitializeCrt178(guard, runtime) ==
                  178 && guard == 0x81u && runtime.call_count == 218,
              "0x562e30 registers nullsub_82 with shared guard");
        runtime.return_value = -178;
        Check(pal4::CrtExitRegistration::InitializeCrt178(guard, runtime) ==
                  -178 && guard == 0x81u && runtime.call_count == 219,
              "0x562e30 repeats registration and forwards signed result");
        runtime.return_value = 179;
        Check(pal4::CrtExitRegistration::InitializeCrt179(guard, runtime) ==
                  179 && guard == 0x81u && runtime.call_count == 220,
              "0x562e60 registers nullsub_82 with shared guard");
        runtime.return_value = -179;
        Check(pal4::CrtExitRegistration::InitializeCrt179(guard, runtime) ==
                  -179 && guard == 0x81u && runtime.call_count == 221,
              "0x562e60 repeats registration and forwards signed result");
        runtime.return_value = 180;
        Check(pal4::CrtExitRegistration::InitializeCrt180(guard, runtime) ==
                  180 && guard == 0x81u && runtime.call_count == 222,
              "0x562f60 registers nullsub_82 with shared guard");
        runtime.return_value = -180;
        Check(pal4::CrtExitRegistration::InitializeCrt180(guard, runtime) ==
                  -180 && guard == 0x81u && runtime.call_count == 223,
              "0x562f60 repeats registration and forwards signed result");
        runtime.return_value = 181;
        Check(pal4::CrtExitRegistration::InitializeCrt181(guard, runtime) ==
                  181 && guard == 0x81u && runtime.call_count == 224,
              "0x562f90 registers nullsub_82 with shared guard");
        runtime.return_value = -181;
        Check(pal4::CrtExitRegistration::InitializeCrt181(guard, runtime) ==
                  -181 && guard == 0x81u && runtime.call_count == 225,
              "0x562f90 repeats registration and forwards signed result");
        runtime.return_value = 182;
        Check(pal4::CrtExitRegistration::InitializeCrt182(guard, runtime) ==
                  182 && guard == 0x81u && runtime.call_count == 226,
              "0x563760 registers nullsub_82 with shared guard");
        runtime.return_value = -182;
        Check(pal4::CrtExitRegistration::InitializeCrt182(guard, runtime) ==
                  -182 && guard == 0x81u && runtime.call_count == 227,
              "0x563760 repeats registration and forwards signed result");
        runtime.return_value = 183;
        Check(pal4::CrtExitRegistration::InitializeCrt183(guard, runtime) ==
                  183 && guard == 0x81u && runtime.call_count == 228,
              "0x563790 registers nullsub_82 with shared guard");
        runtime.return_value = -183;
        Check(pal4::CrtExitRegistration::InitializeCrt183(guard, runtime) ==
                  -183 && guard == 0x81u && runtime.call_count == 229,
              "0x563790 repeats registration and forwards signed result");
        runtime.return_value = 184;
        Check(pal4::CrtExitRegistration::InitializeCrt184(guard, runtime) ==
                  184 && guard == 0x81u && runtime.call_count == 230,
              "0x563a60 registers nullsub_82 with shared guard");
        runtime.return_value = -184;
        Check(pal4::CrtExitRegistration::InitializeCrt184(guard, runtime) ==
                  -184 && guard == 0x81u && runtime.call_count == 231,
              "0x563a60 repeats registration and forwards signed result");
        runtime.return_value = 185;
        Check(pal4::CrtExitRegistration::InitializeCrt185(guard, runtime) ==
                  185 && guard == 0x81u && runtime.call_count == 232,
              "0x5659d0 registers nullsub_82 with shared guard");
        runtime.return_value = -185;
        Check(pal4::CrtExitRegistration::InitializeCrt185(guard, runtime) ==
                  -185 && guard == 0x81u && runtime.call_count == 233,
              "0x5659d0 repeats registration and forwards signed result");
        runtime.return_value = 186;
        Check(pal4::CrtExitRegistration::InitializeCrt186(guard, runtime) ==
                  186 && guard == 0x81u && runtime.call_count == 234,
              "0x566180 registers nullsub_82 with shared guard");
        runtime.return_value = -186;
        Check(pal4::CrtExitRegistration::InitializeCrt186(guard, runtime) ==
                  -186 && guard == 0x81u && runtime.call_count == 235,
              "0x566180 repeats registration and forwards signed result");
        runtime.return_value = 187;
        Check(pal4::CrtExitRegistration::InitializeCrt187(guard, runtime) ==
                  187 && guard == 0x81u && runtime.call_count == 236,
              "0x566780 registers nullsub_82 with shared guard");
        runtime.return_value = -187;
        Check(pal4::CrtExitRegistration::InitializeCrt187(guard, runtime) ==
                  -187 && guard == 0x81u && runtime.call_count == 237,
              "0x566780 repeats registration and forwards signed result");
        runtime.return_value = 188;
        Check(pal4::CrtExitRegistration::InitializeCrt188(guard, runtime) ==
                  188 && guard == 0x81u && runtime.call_count == 238,
              "0x568cc0 registers nullsub_82 with shared guard");
        runtime.return_value = -188;
        Check(pal4::CrtExitRegistration::InitializeCrt188(guard, runtime) ==
                  -188 && guard == 0x81u && runtime.call_count == 239,
              "0x568cc0 repeats registration and forwards signed result");
        runtime.return_value = 189;
        Check(pal4::CrtExitRegistration::InitializeCrt189(guard, runtime) ==
                  189 && guard == 0x81u && runtime.call_count == 240,
              "0x568e00 registers nullsub_82 with shared guard");
        runtime.return_value = -189;
        Check(pal4::CrtExitRegistration::InitializeCrt189(guard, runtime) ==
                  -189 && guard == 0x81u && runtime.call_count == 241,
              "0x568e00 repeats registration and forwards signed result");
        runtime.return_value = 190;
        Check(pal4::CrtExitRegistration::InitializeCrt190(guard, runtime) ==
                  190 && guard == 0x81u && runtime.call_count == 242,
              "0x569400 registers nullsub_82 with shared guard");
        runtime.return_value = -190;
        Check(pal4::CrtExitRegistration::InitializeCrt190(guard, runtime) ==
                  -190 && guard == 0x81u && runtime.call_count == 243,
              "0x569400 repeats registration and forwards signed result");
        runtime.return_value = 191;
        Check(pal4::CrtExitRegistration::InitializeCrt191(guard, runtime) ==
                  191 && guard == 0x81u && runtime.call_count == 244,
              "0x5699a0 registers nullsub_82 with shared guard");
        runtime.return_value = -191;
        Check(pal4::CrtExitRegistration::InitializeCrt191(guard, runtime) ==
                  -191 && guard == 0x81u && runtime.call_count == 245,
              "0x5699a0 repeats registration and forwards signed result");
        runtime.return_value = 192;
        Check(pal4::CrtExitRegistration::InitializeCrt192(guard, runtime) ==
                  192 && guard == 0x81u && runtime.call_count == 246,
              "0x56a6e0 registers nullsub_82 with shared guard");
        runtime.return_value = -192;
        Check(pal4::CrtExitRegistration::InitializeCrt192(guard, runtime) ==
                  -192 && guard == 0x81u && runtime.call_count == 247,
              "0x56a6e0 repeats registration and forwards signed result");
        runtime.return_value = 193;
        Check(pal4::CrtExitRegistration::InitializeCrt193(guard, runtime) ==
                  193 && guard == 0x81u && runtime.call_count == 248,
              "0x56b130 registers nullsub_82 with shared guard");
        runtime.return_value = -193;
        Check(pal4::CrtExitRegistration::InitializeCrt193(guard, runtime) ==
                  -193 && guard == 0x81u && runtime.call_count == 249,
              "0x56b130 repeats registration and forwards signed result");
        runtime.return_value = 194;
        Check(pal4::CrtExitRegistration::InitializeCrt194(guard, runtime) ==
                  194 && guard == 0x81u && runtime.call_count == 250,
              "0x56bd00 registers nullsub_82 with shared guard");
        runtime.return_value = -194;
        Check(pal4::CrtExitRegistration::InitializeCrt194(guard, runtime) ==
                  -194 && guard == 0x81u && runtime.call_count == 251,
              "0x56bd00 repeats registration and forwards signed result");
        runtime.return_value = 195;
        Check(pal4::CrtExitRegistration::InitializeCrt195(guard, runtime) ==
                  195 && guard == 0x81u && runtime.call_count == 252,
              "0x56bf50 registers nullsub_82 with shared guard");
        runtime.return_value = -195;
        Check(pal4::CrtExitRegistration::InitializeCrt195(guard, runtime) ==
                  -195 && guard == 0x81u && runtime.call_count == 253,
              "0x56bf50 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt196(
                  guard, runtime) == 37 && guard == 0x81u &&
                  runtime.call_count == 1,
              "0x56cda0 sets shared guard and registers nullsub_82");
        runtime.return_value = -196;
        Check(pal4::CrtExitRegistration::InitializeCrt196(
                  guard, runtime) == -196 && guard == 0x81u &&
                  runtime.call_count == 2,
              "0x56cda0 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x40u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt197(
                  guard, runtime) == 37 && guard == 0x41u &&
                  runtime.call_count == 1,
              "0x56d980 sets shared guard and registers nullsub_82");
        runtime.return_value = -197;
        Check(pal4::CrtExitRegistration::InitializeCrt197(
                  guard, runtime) == -197 && guard == 0x41u &&
                  runtime.call_count == 2,
              "0x56d980 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x20u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt198(
                  guard, runtime) == 37 && guard == 0x21u &&
                  runtime.call_count == 1,
              "0x56da60 sets shared guard and registers nullsub_82");
        runtime.return_value = -198;
        Check(pal4::CrtExitRegistration::InitializeCrt198(
                  guard, runtime) == -198 && guard == 0x21u &&
                  runtime.call_count == 2,
              "0x56da60 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x10u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt199(
                  guard, runtime) == 37 && guard == 0x11u &&
                  runtime.call_count == 1,
              "0x56dcd0 sets shared guard and registers nullsub_82");
        runtime.return_value = -199;
        Check(pal4::CrtExitRegistration::InitializeCrt199(
                  guard, runtime) == -199 && guard == 0x11u &&
                  runtime.call_count == 2,
              "0x56dcd0 repeats registration and forwards signed result");
    }
    for (const auto entry : {
             &pal4::CrtExitRegistration::InitializeCrt200,
             &pal4::CrtExitRegistration::InitializeCrt201}) {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        Check(entry(guard, runtime) == 37 && guard == 0x81u &&
                  runtime.call_count == 1,
              "script property CRT entry sets shared guard and registers nullsub_82");
        runtime.return_value = -200;
        Check(entry(guard, runtime) == -200 && guard == 0x81u &&
                  runtime.call_count == 2,
              "script property CRT entry repeats registration and forwards result");
    }
    {
        std::array<std::uint8_t, 4> guards{0x80u, 0x40u, 0x20u, 0x10u};
        class ObservingAtexit final : public pal4::ICrtAtexitApi {
        public:
            explicit ObservingAtexit(const std::array<std::uint8_t, 4>& values)
                : guards(values) {}
            int Register(pal4::CrtExitCallback callback) override {
                Check(callback != nullptr, "locale CRT callback is non-null");
                if (first_callback != nullptr)
                    Check(callback == first_callback,
                          "locale entries register the same callback");
                first_callback = callback;
                snapshots.push_back(guards);
                return static_cast<int>(snapshots.size()) * -7;
            }
            const std::array<std::uint8_t, 4>& guards;
            std::vector<std::array<std::uint8_t, 4>> snapshots;
            pal4::CrtExitCallback first_callback = nullptr;
        } runtime(guards);
        Check(pal4::CrtExitRegistration::kFacetGuardOneAddress == 0x978b3cu &&
                  pal4::CrtExitRegistration::kLocaleGuardThreeAddress ==
                      0x978a96u &&
                  pal4::CrtExitRegistration::kLocaleGuardFourAddress ==
                      0x978a97u &&
                  pal4::CrtExitRegistration::kFacetGuardTwoAddress == 0x978b30u,
              "four locale guard addresses match IDA");
        Check(pal4::CrtExitRegistration::InitializeLocaleFour(
                  guards[0], guards[1], guards[2], guards[3], runtime) ==
                  -28 && guards == std::array<std::uint8_t, 4>{
                                      0x81u, 0x41u, 0x21u, 0x11u} &&
                  runtime.snapshots == std::vector<std::array<std::uint8_t, 4>>{
                      {0x81u, 0x40u, 0x20u, 0x10u},
                      {0x81u, 0x41u, 0x20u, 0x10u},
                      {0x81u, 0x41u, 0x21u, 0x10u},
                      {0x81u, 0x41u, 0x21u, 0x11u}},
              "locale guards are set immediately before each registration");
        Check(pal4::CrtExitRegistration::RegisterLocaleOne(runtime) == -35 &&
                  pal4::CrtExitRegistration::RegisterLocaleTwo(runtime) == -42 &&
                  pal4::CrtExitRegistration::RegisterLocaleThree(runtime) == -49,
              "individual locale wrappers always register and forward results");
        Check(pal4::CrtExitRegistration::InitializeCombatLocaleFour(
                  guards[0], guards[1], guards[2], guards[3], runtime) ==
                  -77 && runtime.snapshots.size() == 11u &&
                  runtime.snapshots[7] == guards &&
                  runtime.snapshots[10] == guards,
              "combat locale initializer repeats four registrations with set guards");
        Check(pal4::CrtExitRegistration::RegisterCombatLocaleOne(runtime) ==
                  -84 &&
                  pal4::CrtExitRegistration::RegisterCombatLocaleTwo(runtime) ==
                  -91 &&
                  pal4::CrtExitRegistration::RegisterCombatLocaleThree(runtime) ==
                  -98,
              "combat locale wrappers repeat callback and forward results");
        std::array<std::uint8_t, 4> fresh_guards{0x10u, 0x20u, 0x40u, 0x80u};
        ObservingAtexit fresh_runtime(fresh_guards);
        Check(pal4::CrtExitRegistration::InitializeCombatLocaleFour(
                  fresh_guards[0], fresh_guards[1], fresh_guards[2],
                  fresh_guards[3], fresh_runtime) == -28 &&
                  fresh_runtime.snapshots ==
                      std::vector<std::array<std::uint8_t, 4>>{
                          {0x11u, 0x20u, 0x40u, 0x80u},
                          {0x11u, 0x21u, 0x40u, 0x80u},
                          {0x11u, 0x21u, 0x41u, 0x80u},
                          {0x11u, 0x21u, 0x41u, 0x81u}},
              "combat locale initializer sets each guard before registering");
    }
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt202(
                  guard, runtime) == 37 && guard == 0x81u &&
                  runtime.call_count == 1,
              "0x574130 sets shared guard and registers nullsub_82");
        runtime.return_value = -202;
        Check(pal4::CrtExitRegistration::InitializeCrt202(
                  guard, runtime) == -202 && guard == 0x81u &&
                  runtime.call_count == 2,
              "0x574130 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x40u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt203(
                  guard, runtime) == 37 && guard == 0x41u &&
                  runtime.call_count == 1,
              "0x574dc0 sets shared guard and registers nullsub_82");
        runtime.return_value = -203;
        Check(pal4::CrtExitRegistration::InitializeCrt203(
                  guard, runtime) == -203 && guard == 0x41u &&
                  runtime.call_count == 2,
              "0x574dc0 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x20u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt204(
                  guard, runtime) == 37 && guard == 0x21u &&
                  runtime.call_count == 1,
              "0x576cb0 sets shared guard and registers nullsub_82");
        runtime.return_value = -204;
        Check(pal4::CrtExitRegistration::InitializeCrt204(
                  guard, runtime) == -204 && guard == 0x21u &&
                  runtime.call_count == 2,
              "0x576cb0 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt205(
                  guard, runtime) == 37 && guard == 0x81u &&
                  runtime.call_count == 1,
              "0x579a90 sets guard bit and registers nullsub_82");
        runtime.return_value = -205;
        Check(pal4::CrtExitRegistration::InitializeCrt205(
                  guard, runtime) == -205 && guard == 0x81u &&
                  runtime.call_count == 2,
              "0x579a90 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x40u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt206(
                  guard, runtime) == 37 && guard == 0x41u &&
                  runtime.call_count == 1,
              "0x57a170 sets guard bit and registers nullsub_82");
        runtime.return_value = -206;
        Check(pal4::CrtExitRegistration::InitializeCrt206(
                  guard, runtime) == -206 && guard == 0x41u &&
                  runtime.call_count == 2,
              "0x57a170 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x80u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt207(
                  guard, runtime) == 37 && guard == 0x81u &&
                  runtime.call_count == 1,
              "0x57bed0 sets guard bit and registers nullsub_82");
        runtime.return_value = -207;
        Check(pal4::CrtExitRegistration::InitializeCrt207(
                  guard, runtime) == -207 && guard == 0x81u &&
                  runtime.call_count == 2,
              "0x57bed0 repeats registration and forwards signed result");
    }
    {
        std::uint8_t guard = 0x40u;
        FakeAtexit runtime;
        Check(pal4::CrtExitRegistration::InitializeCrt208(
                  guard, runtime) == 37 && guard == 0x41u &&
                  runtime.call_count == 1,
              "0x57c050 sets the shared guard bit");
        runtime.return_value = -208;
        Check(pal4::CrtExitRegistration::InitializeCrt209(
                  guard, runtime) == -208 && guard == 0x41u &&
                  runtime.call_count == 2,
              "0x57c080 repeats nullsub_82 registration on shared guard");
        runtime.return_value = -210;
        Check(pal4::CrtExitRegistration::InitializeCrt210(
                  guard, runtime) == -210 && guard == 0x41u &&
                  runtime.call_count == 3,
              "0x57ce10 repeats registration with shared guard set");
        runtime.return_value = -211;
        Check(pal4::CrtExitRegistration::InitializeCrt211(
                  guard, runtime) == -211 && guard == 0x41u &&
                  runtime.call_count == 4,
              "0x57cf40 repeats registration and forwards signed result");
        runtime.return_value = -212;
        Check(pal4::CrtExitRegistration::InitializeCrt212(
                  guard, runtime) == -212 && guard == 0x41u &&
                  runtime.call_count == 5,
              "0x57de30 repeats registration with shared guard");
        runtime.return_value = -213;
        Check(pal4::CrtExitRegistration::InitializeCrt213(
                  guard, runtime) == -213 && guard == 0x41u &&
                  runtime.call_count == 6,
              "0x5814d0 repeats registration with shared guard");
        runtime.return_value = -214;
        Check(pal4::CrtExitRegistration::InitializeCrt214(
                  guard, runtime) == -214 && guard == 0x41u &&
                  runtime.call_count == 7,
              "0x583550 repeats registration with shared guard");
        runtime.return_value = -215;
        Check(pal4::CrtExitRegistration::InitializeCrt215(
                  guard, runtime) == -215 && guard == 0x41u &&
                  runtime.call_count == 8,
              "0x58f390 repeats registration with shared guard");
        runtime.return_value = -216;
        Check(pal4::CrtExitRegistration::InitializeCrt216(
                  guard, runtime) == -216 && guard == 0x41u &&
                  runtime.call_count == 9,
              "0x58f900 repeats registration with shared guard");
        runtime.return_value = -217;
        Check(pal4::CrtExitRegistration::InitializeCrt217(
                  guard, runtime) == -217 && guard == 0x41u &&
                  runtime.call_count == 10,
              "0x58fd00 repeats registration with shared guard");
        runtime.return_value = -218;
        Check(pal4::CrtExitRegistration::InitializeCrt218(
                  guard, runtime) == -218 && guard == 0x41u &&
                  runtime.call_count == 11,
              "0x58fe10 repeats registration with shared guard");
        runtime.return_value = -219;
        Check(pal4::CrtExitRegistration::InitializeCrt219(
                  guard, runtime) == -219 && guard == 0x41u &&
                  runtime.call_count == 12,
              "0x58ff00 repeats registration with shared guard");
        runtime.return_value = -220;
        Check(pal4::CrtExitRegistration::InitializeCrt220(
                  guard, runtime) == -220 && guard == 0x41u &&
                  runtime.call_count == 13,
              "0x5900e0 repeats registration with shared guard");
        runtime.return_value = -221;
        Check(pal4::CrtExitRegistration::InitializeCrt221(
                  guard, runtime) == -221 && guard == 0x41u &&
                  runtime.call_count == 14,
              "0x590270 repeats registration with shared guard");
        runtime.return_value = -222;
        Check(pal4::CrtExitRegistration::InitializeCrt222(
                  guard, runtime) == -222 && guard == 0x41u &&
                  runtime.call_count == 15,
              "0x590430 repeats registration with shared guard");
        runtime.return_value = -223;
        Check(pal4::CrtExitRegistration::InitializeCrt223(
                  guard, runtime) == -223 && guard == 0x41u &&
                  runtime.call_count == 16,
              "0x5905b0 repeats registration with shared guard");
        runtime.return_value = -224;
        Check(pal4::CrtExitRegistration::InitializeCrt224(
                  guard, runtime) == -224 && guard == 0x41u &&
                  runtime.call_count == 17,
              "0x590720 repeats registration with shared guard");
        runtime.return_value = -225;
        Check(pal4::CrtExitRegistration::InitializeCrt225(
                  guard, runtime) == -225 && guard == 0x41u &&
                  runtime.call_count == 18,
              "0x5925a0 repeats registration with shared guard");
        runtime.return_value = -226;
        Check(pal4::CrtExitRegistration::InitializeCrt226(
                  guard, runtime) == -226 && guard == 0x41u &&
                  runtime.call_count == 19,
              "0x5925d0 repeats registration with shared guard");
        runtime.return_value = -227;
        Check(pal4::CrtExitRegistration::InitializeCrt227(
                  guard, runtime) == -227 && guard == 0x41u &&
                  runtime.call_count == 20,
              "0x593200 repeats registration with shared guard");
        runtime.return_value = -228;
        Check(pal4::CrtExitRegistration::InitializeCrt228(
                  guard, runtime) == -228 && guard == 0x41u &&
                  runtime.call_count == 21,
              "0x593230 repeats registration with shared guard");
        runtime.return_value = -229;
        Check(pal4::CrtExitRegistration::InitializeCrt229(
                  guard, runtime) == -229 && guard == 0x41u &&
                  runtime.call_count == 22,
              "0x593b00 repeats registration with shared guard");
        runtime.return_value = -230;
        Check(pal4::CrtExitRegistration::InitializeCrt230(
                  guard, runtime) == -230 && guard == 0x41u &&
                  runtime.call_count == 23,
              "0x593f90 repeats registration with shared guard");
        runtime.return_value = -231;
        Check(pal4::CrtExitRegistration::InitializeCrt231(
                  guard, runtime) == -231 && guard == 0x41u &&
                  runtime.call_count == 24,
              "0x594600 repeats registration with shared guard");
        runtime.return_value = -232;
        Check(pal4::CrtExitRegistration::InitializeCrt232(
                  guard, runtime) == -232 && guard == 0x41u &&
                  runtime.call_count == 25,
              "0x594920 repeats registration with shared guard");
        runtime.return_value = -233;
        Check(pal4::CrtExitRegistration::InitializeCrt233(
                  guard, runtime) == -233 && guard == 0x41u &&
                  runtime.call_count == 26,
              "0x594d40 repeats registration with shared guard");
        runtime.return_value = -234;
        Check(pal4::CrtExitRegistration::InitializeCrt234(
                  guard, runtime) == -234 && guard == 0x41u &&
                  runtime.call_count == 27,
              "0x595120 repeats registration with shared guard");
        runtime.return_value = -235;
        Check(pal4::CrtExitRegistration::InitializeCrt235(
                  guard, runtime) == -235 && guard == 0x41u &&
                  runtime.call_count == 28,
              "0x5959f0 repeats registration with shared guard");
        runtime.return_value = -236;
        Check(pal4::CrtExitRegistration::InitializeCrt236(
                  guard, runtime) == -236 && guard == 0x41u &&
                  runtime.call_count == 29,
              "0x595fa0 repeats registration with shared guard");
        runtime.return_value = -237;
        Check(pal4::CrtExitRegistration::InitializeCrt237(
                  guard, runtime) == -237 && guard == 0x41u &&
                  runtime.call_count == 30,
              "0x596050 repeats registration with shared guard");
        runtime.return_value = -238;
        Check(pal4::CrtExitRegistration::InitializeCrt238(
                  guard, runtime) == -238 && guard == 0x41u &&
                  runtime.call_count == 31,
              "0x596a20 repeats registration with shared guard");
        runtime.return_value = -239;
        Check(pal4::CrtExitRegistration::InitializeCrt239(
                  guard, runtime) == -239 && guard == 0x41u &&
                  runtime.call_count == 32,
              "0x5975e0 repeats registration with shared guard");
        runtime.return_value = -240;
        Check(pal4::CrtExitRegistration::InitializeCrt240(
                  guard, runtime) == -240 && guard == 0x41u &&
                  runtime.call_count == 33,
              "0x598140 repeats registration with shared guard");
        runtime.return_value = -241;
        Check(pal4::CrtExitRegistration::InitializeCrt241(
                  guard, runtime) == -241 && guard == 0x41u &&
                  runtime.call_count == 34,
              "0x598220 repeats registration with shared guard");
        runtime.return_value = -242;
        Check(pal4::CrtExitRegistration::InitializeCrt242(
                  guard, runtime) == -242 && guard == 0x41u &&
                  runtime.call_count == 35,
              "0x598350 repeats registration with shared guard");
        runtime.return_value = -243;
        Check(pal4::CrtExitRegistration::InitializeCrt243(
                  guard, runtime) == -243 && guard == 0x41u &&
                  runtime.call_count == 36,
              "0x598e00 repeats registration with shared guard");
        runtime.return_value = -244;
        Check(pal4::CrtExitRegistration::InitializeCrt244(
                  guard, runtime) == -244 && guard == 0x41u &&
                  runtime.call_count == 37,
              "0x599440 repeats registration with shared guard");
        runtime.return_value = -245;
        Check(pal4::CrtExitRegistration::InitializeCrt245(
                  guard, runtime) == -245 && guard == 0x41u &&
                  runtime.call_count == 38,
              "0x599870 repeats registration with shared guard");
        runtime.return_value = -246;
        Check(pal4::CrtExitRegistration::InitializeCrt246(
                  guard, runtime) == -246 && guard == 0x41u &&
                  runtime.call_count == 39,
              "0x599c70 repeats registration with shared guard");
        runtime.return_value = -247;
        Check(pal4::CrtExitRegistration::InitializeCrt247(
                  guard, runtime) == -247 && guard == 0x41u &&
                  runtime.call_count == 40,
              "0x599e70 repeats registration with shared guard");
        runtime.return_value = -248;
        Check(pal4::CrtExitRegistration::InitializeCrt248(
                  guard, runtime) == -248 && guard == 0x41u &&
                  runtime.call_count == 41,
              "0x599f00 repeats registration with shared guard");
        runtime.return_value = -249;
        Check(pal4::CrtExitRegistration::InitializeCrt249(
                  guard, runtime) == -249 && guard == 0x41u &&
                  runtime.call_count == 42,
              "0x59a9c0 repeats registration with shared guard");
        runtime.return_value = -250;
        Check(pal4::CrtExitRegistration::InitializeCrt250(
                  guard, runtime) == -250 && guard == 0x41u &&
                  runtime.call_count == 43,
              "0x59b730 repeats registration with shared guard");
        runtime.return_value = -251;
        Check(pal4::CrtExitRegistration::InitializeCrt251(
                  guard, runtime) == -251 && guard == 0x41u &&
                  runtime.call_count == 44,
              "0x59c420 repeats registration with shared guard");
        runtime.return_value = -252;
        Check(pal4::CrtExitRegistration::InitializeCrt252(
                  guard, runtime) == -252 && guard == 0x41u &&
                  runtime.call_count == 45,
              "0x59d0d0 repeats registration with shared guard");
        runtime.return_value = -253;
        Check(pal4::CrtExitRegistration::InitializeCrt253(
                  guard, runtime) == -253 && guard == 0x41u &&
                  runtime.call_count == 46,
              "0x59e670 repeats registration with shared guard");
        runtime.return_value = -254;
        Check(pal4::CrtExitRegistration::InitializeCrt254(
                  guard, runtime) == -254 && guard == 0x41u &&
                  runtime.call_count == 47,
              "0x59f350 repeats registration with shared guard");
        runtime.return_value = -255;
        Check(pal4::CrtExitRegistration::InitializeCrt255(
                  guard, runtime) == -255 && guard == 0x41u &&
                  runtime.call_count == 48,
              "0x59fbc0 repeats registration with shared guard");
        runtime.return_value = -256;
        Check(pal4::CrtExitRegistration::InitializeCrt256(
                  guard, runtime) == -256 && guard == 0x41u &&
                  runtime.call_count == 49,
              "0x5a3c90 repeats registration with shared guard");
        runtime.return_value = -257;
        Check(pal4::CrtExitRegistration::InitializeCrt257(
                  guard, runtime) == -257 && guard == 0x41u &&
                  runtime.call_count == 50,
              "0x5a42b0 repeats registration with shared guard");
        runtime.return_value = -258;
        Check(pal4::CrtExitRegistration::InitializeCrt258(
                  guard, runtime) == -258 && guard == 0x41u &&
                  runtime.call_count == 51,
              "0x5a4c20 repeats registration with shared guard");
        runtime.return_value = -259;
        Check(pal4::CrtExitRegistration::InitializeCrt259(
                  guard, runtime) == -259 && guard == 0x41u &&
                  runtime.call_count == 52,
              "0x5a5f00 repeats registration with shared guard");
        runtime.return_value = -260;
        Check(pal4::CrtExitRegistration::InitializeCrt260(
                  guard, runtime) == -260 && guard == 0x41u &&
                  runtime.call_count == 53,
              "0x5a6ca0 repeats registration with shared guard");
        runtime.return_value = -261;
        Check(pal4::CrtExitRegistration::InitializeCrt261(
                  guard, runtime) == -261 && guard == 0x41u &&
                  runtime.call_count == 54,
              "0x5a6cd0 repeats registration with shared guard");
        runtime.return_value = -262;
        Check(pal4::CrtExitRegistration::InitializeCrt262(
                  guard, runtime) == -262 && guard == 0x41u &&
                  runtime.call_count == 55,
              "0x5a9290 repeats registration with shared guard");
        runtime.return_value = -263;
        Check(pal4::CrtExitRegistration::InitializeCrt263(
                  guard, runtime) == -263 && guard == 0x41u &&
                  runtime.call_count == 56,
              "0x5a9340 repeats registration with shared guard");
        runtime.return_value = -264;
        Check(pal4::CrtExitRegistration::InitializeCrt264(
                  guard, runtime) == -264 && guard == 0x41u &&
                  runtime.call_count == 57,
              "0x5a93a0 repeats registration with shared guard");
        runtime.return_value = -265;
        Check(pal4::CrtExitRegistration::InitializeCrt265(
                  guard, runtime) == -265 && guard == 0x41u &&
                  runtime.call_count == 58,
              "0x5a9440 repeats registration with shared guard");
        runtime.return_value = -266;
        Check(pal4::CrtExitRegistration::InitializeCrt266(
                  guard, runtime) == -266 && guard == 0x41u &&
                  runtime.call_count == 59,
              "0x5a94e0 repeats registration with shared guard");
        runtime.return_value = -267;
        Check(pal4::CrtExitRegistration::InitializeCrt267(
                  guard, runtime) == -267 && guard == 0x41u &&
                  runtime.call_count == 60,
              "0x5a9b40 repeats registration with shared guard");
        runtime.return_value = -268;
        Check(pal4::CrtExitRegistration::InitializeCrt268(
                  guard, runtime) == -268 && guard == 0x41u &&
                  runtime.call_count == 61,
              "0x5a9b80 repeats registration with shared guard");
        runtime.return_value = -269;
        Check(pal4::CrtExitRegistration::InitializeCrt269(
                  guard, runtime) == -269 && guard == 0x41u &&
                  runtime.call_count == 62,
              "0x5a9c70 repeats registration with shared guard");
        runtime.return_value = -270;
        Check(pal4::CrtExitRegistration::InitializeCrt270(
                  guard, runtime) == -270 && guard == 0x41u &&
                  runtime.call_count == 63,
              "0x5a9ea0 repeats registration with shared guard");
        runtime.return_value = -271;
        Check(pal4::CrtExitRegistration::InitializeCrt271(
                  guard, runtime) == -271 && guard == 0x41u &&
                  runtime.call_count == 64,
              "0x5aa110 repeats registration with shared guard");
        runtime.return_value = -272;
        Check(pal4::CrtExitRegistration::InitializeCrt272(
                  guard, runtime) == -272 && guard == 0x41u &&
                  runtime.call_count == 65,
              "0x5aa770 repeats registration with shared guard");
        runtime.return_value = -273;
        Check(pal4::CrtExitRegistration::InitializeCrt273(
                  guard, runtime) == -273 && guard == 0x41u &&
                  runtime.call_count == 66,
              "0x5aad80 repeats registration with shared guard");
        runtime.return_value = -274;
        Check(pal4::CrtExitRegistration::InitializeCrt274(
                  guard, runtime) == -274 && guard == 0x41u &&
                  runtime.call_count == 67,
              "0x5ab2c0 repeats registration with shared guard");
        runtime.return_value = -275;
        Check(pal4::CrtExitRegistration::InitializeCrt275(
                  guard, runtime) == -275 && guard == 0x41u &&
                  runtime.call_count == 68,
              "0x5acf20 repeats registration with shared guard");
        runtime.return_value = -276;
        Check(pal4::CrtExitRegistration::InitializeCrt276(
                  guard, runtime) == -276 && guard == 0x41u &&
                  runtime.call_count == 69,
              "0x5ad130 repeats registration with shared guard");
        runtime.return_value = -277;
        Check(pal4::CrtExitRegistration::InitializeCrt277(
                  guard, runtime) == -277 && guard == 0x41u &&
                  runtime.call_count == 70,
              "0x5b4540 repeats registration with shared guard");
        runtime.return_value = -278;
        Check(pal4::CrtExitRegistration::InitializeCrt278(
                  guard, runtime) == -278 && guard == 0x41u &&
                  runtime.call_count == 71,
              "0x5b4e90 repeats registration with shared guard");
        runtime.return_value = -279;
        Check(pal4::CrtExitRegistration::InitializeCrt279(
                  guard, runtime) == -279 && guard == 0x41u &&
                  runtime.call_count == 72,
              "0x5b4ec0 repeats registration with shared guard");
        runtime.return_value = -280;
        Check(pal4::CrtExitRegistration::InitializeCrt280(
                  guard, runtime) == -280 && guard == 0x41u &&
                  runtime.call_count == 73,
              "0x5b5ac0 repeats registration with shared guard");
        runtime.return_value = -281;
        Check(pal4::CrtExitRegistration::InitializeCrt281(
                  guard, runtime) == -281 && guard == 0x41u &&
                  runtime.call_count == 74,
              "0x5b7400 repeats registration with shared guard");
        std::uint8_t fresh_guard = 0u;
        Check(pal4::CrtExitRegistration::InitializeCrt281(
                  fresh_guard, runtime) == -281 && fresh_guard == 1u &&
                  runtime.call_count == 75,
              "0x5b7400 sets bit zero on first registration");
        runtime.return_value = -282;
        Check(pal4::CrtExitRegistration::InitializeCrt282(
                  guard, runtime) == -282 && guard == 0x41u &&
                  runtime.call_count == 76,
              "0x5bede0 repeats registration with shared guard");
        std::uint8_t new_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt282(
                  new_guard, runtime) == -282 && new_guard == 0x81u &&
                  runtime.call_count == 77,
              "0x5bede0 sets the guard bit and preserves other bits");
        runtime.return_value = -283;
        Check(pal4::CrtExitRegistration::InitializeCrt283(
                  guard, runtime) == -283 && guard == 0x41u &&
                  runtime.call_count == 78,
              "0x5c4400 repeats registration with shared guard");
        std::uint8_t sixth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt283(
                  sixth_guard, runtime) == -283 &&
                  sixth_guard == 0x81u && runtime.call_count == 79,
              "0x5c4400 sets the low guard bit and preserves other bits");
        runtime.return_value = -284;
        Check(pal4::CrtExitRegistration::InitializeCrt284(
                  guard, runtime) == -284 && guard == 0x41u &&
                  runtime.call_count == 80,
              "0x5c8470 repeats registration with shared guard");
        std::uint8_t seventh_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt284(
                  seventh_guard, runtime) == -284 &&
                  seventh_guard == 0x81u && runtime.call_count == 81,
              "0x5c8470 sets the low guard bit and preserves other bits");
        runtime.return_value = -285;
        Check(pal4::CrtExitRegistration::InitializeCrt285(
                  guard, runtime) == -285 && guard == 0x41u &&
                  runtime.call_count == 82,
              "0x5ccd80 repeats registration with shared guard");
        std::uint8_t eighth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt285(
                  eighth_guard, runtime) == -285 &&
                  eighth_guard == 0x81u && runtime.call_count == 83,
              "0x5ccd80 sets the low guard bit and preserves other bits");
        runtime.return_value = -286;
        Check(pal4::CrtExitRegistration::InitializeCrt286(
                  guard, runtime) == -286 && guard == 0x41u &&
                  runtime.call_count == 84,
              "0x5cf580 repeats registration with shared guard");
        std::uint8_t ninth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt286(
                  ninth_guard, runtime) == -286 &&
                  ninth_guard == 0x81u && runtime.call_count == 85,
              "0x5cf580 preserves unrelated guard bits");
        runtime.return_value = -287;
        Check(pal4::CrtExitRegistration::InitializeCrt287(
                  guard, runtime) == -287 && guard == 0x41u &&
                  runtime.call_count == 86,
              "0x5d8c00 repeats registration with shared guard");
        std::uint8_t tenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt287(
                  tenth_guard, runtime) == -287 &&
                  tenth_guard == 0x81u && runtime.call_count == 87,
              "0x5d8c00 preserves unrelated guard bits");
        runtime.return_value = -288;
        Check(pal4::CrtExitRegistration::InitializeCrt288(
                  guard, runtime) == -288 && guard == 0x41u &&
                  runtime.call_count == 88,
              "0x5e8f90 repeats registration with shared guard");
        std::uint8_t eleventh_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt288(
                  eleventh_guard, runtime) == -288 &&
                  eleventh_guard == 0x81u && runtime.call_count == 89,
              "0x5e8f90 preserves unrelated guard bits");
        runtime.return_value = -289;
        Check(pal4::CrtExitRegistration::InitializeCrt289(
                  guard, runtime) == -289 && guard == 0x41u &&
                  runtime.call_count == 90,
              "0x5eaa40 repeats registration with shared guard");
        std::uint8_t twelfth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt289(
                  twelfth_guard, runtime) == -289 &&
                  twelfth_guard == 0x81u && runtime.call_count == 91,
              "0x5eaa40 preserves unrelated guard bits");
        runtime.return_value = -290;
        Check(pal4::CrtExitRegistration::InitializeCrt290(
                  guard, runtime) == -290 && guard == 0x41u &&
                  runtime.call_count == 92,
              "0x5eacb0 repeats registration with shared guard");
        std::uint8_t thirteenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt290(
                  thirteenth_guard, runtime) == -290 &&
                  thirteenth_guard == 0x81u && runtime.call_count == 93,
              "0x5eacb0 preserves unrelated guard bits");
        runtime.return_value = -291;
        Check(pal4::CrtExitRegistration::InitializeCrt291(
                  guard, runtime) == -291 && guard == 0x41u &&
                  runtime.call_count == 94,
              "0x5ead80 repeats registration with shared guard");
        std::uint8_t fourteenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt291(
                  fourteenth_guard, runtime) == -291 &&
                  fourteenth_guard == 0x81u && runtime.call_count == 95,
              "0x5ead80 preserves unrelated guard bits");
        runtime.return_value = -292;
        Check(pal4::CrtExitRegistration::InitializeCrt292(
                  guard, runtime) == -292 && guard == 0x41u &&
                  runtime.call_count == 96,
              "0x5eadb0 repeats registration with shared guard");
        std::uint8_t fifteenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt292(
                  fifteenth_guard, runtime) == -292 &&
                  fifteenth_guard == 0x81u && runtime.call_count == 97,
              "0x5eadb0 preserves unrelated guard bits");
        runtime.return_value = -293;
        Check(pal4::CrtExitRegistration::InitializeCrt293(
                  guard, runtime) == -293 && guard == 0x41u &&
                  runtime.call_count == 98,
              "0x5ec290 repeats registration with shared guard");
        std::uint8_t sixteenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt293(
                  sixteenth_guard, runtime) == -293 &&
                  sixteenth_guard == 0x81u && runtime.call_count == 99,
              "0x5ec290 preserves unrelated guard bits");
        runtime.return_value = -294;
        Check(pal4::CrtExitRegistration::InitializeCrt294(
                  guard, runtime) == -294 && guard == 0x41u &&
                  runtime.call_count == 100,
              "0x5ec2c0 repeats registration with shared guard");
        std::uint8_t seventeenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt294(
                  seventeenth_guard, runtime) == -294 &&
                  seventeenth_guard == 0x81u && runtime.call_count == 101,
              "0x5ec2c0 preserves unrelated guard bits");
        runtime.return_value = -295;
        Check(pal4::CrtExitRegistration::InitializeCrt295(
                  guard, runtime) == -295 && guard == 0x41u &&
                  runtime.call_count == 102,
              "0x5ec5e0 repeats registration with shared guard");
        std::uint8_t eighteenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt295(
                  eighteenth_guard, runtime) == -295 &&
                  eighteenth_guard == 0x81u && runtime.call_count == 103,
              "0x5ec5e0 preserves unrelated guard bits");
        runtime.return_value = -296;
        Check(pal4::CrtExitRegistration::InitializeCrt296(
                  guard, runtime) == -296 && guard == 0x41u &&
                  runtime.call_count == 104,
              "0x5ec710 repeats registration with shared guard");
        std::uint8_t nineteenth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt296(
                  nineteenth_guard, runtime) == -296 &&
                  nineteenth_guard == 0x81u && runtime.call_count == 105,
              "0x5ec710 preserves unrelated guard bits");
        runtime.return_value = -297;
        Check(pal4::CrtExitRegistration::InitializeCrt297(
                  guard, runtime) == -297 && guard == 0x41u &&
                  runtime.call_count == 106,
              "0x5ec740 repeats registration with shared guard");
        std::uint8_t twentieth_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt297(
                  twentieth_guard, runtime) == -297 &&
                  twentieth_guard == 0x81u && runtime.call_count == 107,
              "0x5ec740 preserves unrelated guard bits");
        runtime.return_value = -298;
        Check(pal4::CrtExitRegistration::InitializeCrt298(
                  guard, runtime) == -298 && guard == 0x41u &&
                  runtime.call_count == 108,
              "0x5ec770 repeats registration with shared guard");
        std::uint8_t twenty_first_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt298(
                  twenty_first_guard, runtime) == -298 &&
                  twenty_first_guard == 0x81u && runtime.call_count == 109,
              "0x5ec770 preserves unrelated guard bits");
        runtime.return_value = -299;
        Check(pal4::CrtExitRegistration::InitializeCrt299(
                  guard, runtime) == -299 && guard == 0x41u &&
                  runtime.call_count == 110,
              "0x5ec830 repeats registration with shared guard");
        std::uint8_t twenty_second_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt299(
                  twenty_second_guard, runtime) == -299 &&
                  twenty_second_guard == 0x81u && runtime.call_count == 111,
              "0x5ec830 preserves unrelated guard bits");
        runtime.return_value = -300;
        Check(pal4::CrtExitRegistration::InitializeCrt300(
                  guard, runtime) == -300 && guard == 0x41u &&
                  runtime.call_count == 112,
              "0x5ec950 repeats registration with shared guard");
        std::uint8_t twenty_third_guard = 0x80u;
        Check(pal4::CrtExitRegistration::InitializeCrt300(
                  twenty_third_guard, runtime) == -300 &&
                  twenty_third_guard == 0x81u && runtime.call_count == 113,
              "0x5ec950 preserves unrelated guard bits");
        using Initializer = int (*)(std::uint8_t&, pal4::ICrtAtexitApi&);
        const std::array<Initializer, 178> later_initializers{
            &pal4::CrtExitRegistration::InitializeCrt301,
            &pal4::CrtExitRegistration::InitializeCrt302,
            &pal4::CrtExitRegistration::InitializeCrt303,
            &pal4::CrtExitRegistration::InitializeCrt304,
            &pal4::CrtExitRegistration::InitializeCrt305,
            &pal4::CrtExitRegistration::InitializeCrt306,
            &pal4::CrtExitRegistration::InitializeCrt307,
            &pal4::CrtExitRegistration::InitializeCrt308,
            &pal4::CrtExitRegistration::InitializeCrt309,
            &pal4::CrtExitRegistration::InitializeCrt310,
            &pal4::CrtExitRegistration::InitializeCrt311,
            &pal4::CrtExitRegistration::InitializeCrt312,
            &pal4::CrtExitRegistration::InitializeCrt313,
            &pal4::CrtExitRegistration::InitializeCrt314,
            &pal4::CrtExitRegistration::InitializeCrt315,
            &pal4::CrtExitRegistration::InitializeCrt316,
            &pal4::CrtExitRegistration::InitializeCrt317,
            &pal4::CrtExitRegistration::InitializeCrt318,
            &pal4::CrtExitRegistration::InitializeCrt319,
            &pal4::CrtExitRegistration::InitializeCrt320,
            &pal4::CrtExitRegistration::InitializeCrt321,
            &pal4::CrtExitRegistration::InitializeCrt322,
            &pal4::CrtExitRegistration::InitializeCrt323,
            &pal4::CrtExitRegistration::InitializeCrt324,
            &pal4::CrtExitRegistration::InitializeCrt325,
            &pal4::CrtExitRegistration::InitializeCrt326,
            &pal4::CrtExitRegistration::InitializeCrt327,
            &pal4::CrtExitRegistration::InitializeCrt328,
            &pal4::CrtExitRegistration::InitializeCrt329,
            &pal4::CrtExitRegistration::InitializeCrt330,
            &pal4::CrtExitRegistration::InitializeCrt331,
            &pal4::CrtExitRegistration::InitializeCrt332,
            &pal4::CrtExitRegistration::InitializeCrt333,
            &pal4::CrtExitRegistration::InitializeCrt334,
            &pal4::CrtExitRegistration::InitializeCrt335,
            &pal4::CrtExitRegistration::InitializeCrt336,
            &pal4::CrtExitRegistration::InitializeCrt337,
            &pal4::CrtExitRegistration::InitializeCrt338,
            &pal4::CrtExitRegistration::InitializeCrt339,
            &pal4::CrtExitRegistration::InitializeCrt340,
            &pal4::CrtExitRegistration::InitializeCrt341,
            &pal4::CrtExitRegistration::InitializeCrt342,
            &pal4::CrtExitRegistration::InitializeCrt343,
            &pal4::CrtExitRegistration::InitializeCrt344,
            &pal4::CrtExitRegistration::InitializeCrt345,
            &pal4::CrtExitRegistration::InitializeCrt346,
            &pal4::CrtExitRegistration::InitializeCrt347,
            &pal4::CrtExitRegistration::InitializeCrt348,
            &pal4::CrtExitRegistration::InitializeCrt349,
            &pal4::CrtExitRegistration::InitializeCrt350,
            &pal4::CrtExitRegistration::InitializeCrt351,
            &pal4::CrtExitRegistration::InitializeCrt352,
            &pal4::CrtExitRegistration::InitializeCrt353,
            &pal4::CrtExitRegistration::InitializeCrt354,
            &pal4::CrtExitRegistration::InitializeCrt355,
            &pal4::CrtExitRegistration::InitializeCrt356,
            &pal4::CrtExitRegistration::InitializeCrt357,
            &pal4::CrtExitRegistration::InitializeCrt358,
            &pal4::CrtExitRegistration::InitializeCrt359,
            &pal4::CrtExitRegistration::InitializeCrt360,
            &pal4::CrtExitRegistration::InitializeCrt361,
            &pal4::CrtExitRegistration::InitializeCrt362,
            &pal4::CrtExitRegistration::InitializeCrt363,
            &pal4::CrtExitRegistration::InitializeCrt364,
            &pal4::CrtExitRegistration::InitializeCrt365,
            &pal4::CrtExitRegistration::InitializeCrt366,
            &pal4::CrtExitRegistration::InitializeCrt367,
            &pal4::CrtExitRegistration::InitializeCrt368,
            &pal4::CrtExitRegistration::InitializeCrt369,
            &pal4::CrtExitRegistration::InitializeCrt370,
            &pal4::CrtExitRegistration::InitializeCrt371,
            &pal4::CrtExitRegistration::InitializeCrt372,
            &pal4::CrtExitRegistration::InitializeCrt373,
            &pal4::CrtExitRegistration::InitializeCrt374,
            &pal4::CrtExitRegistration::InitializeCrt375,
            &pal4::CrtExitRegistration::InitializeCrt376,
            &pal4::CrtExitRegistration::InitializeCrt377,
            &pal4::CrtExitRegistration::InitializeCrt378,
            &pal4::CrtExitRegistration::InitializeCrt379,
            &pal4::CrtExitRegistration::InitializeCrt380,
            &pal4::CrtExitRegistration::InitializeCrt381,
            &pal4::CrtExitRegistration::InitializeCrt382,
            &pal4::CrtExitRegistration::InitializeCrt383,
            &pal4::CrtExitRegistration::InitializeCrt384,
            &pal4::CrtExitRegistration::InitializeCrt385,
            &pal4::CrtExitRegistration::InitializeCrt386,
            &pal4::CrtExitRegistration::InitializeCrt387,
            &pal4::CrtExitRegistration::InitializeCrt388,
            &pal4::CrtExitRegistration::InitializeCrt389,
            &pal4::CrtExitRegistration::InitializeCrt390,
            &pal4::CrtExitRegistration::InitializeCrt391,
            &pal4::CrtExitRegistration::InitializeCrt392,
            &pal4::CrtExitRegistration::InitializeCrt393,
            &pal4::CrtExitRegistration::InitializeCrt394,
            &pal4::CrtExitRegistration::InitializeCrt395,
            &pal4::CrtExitRegistration::InitializeCrt396,
            &pal4::CrtExitRegistration::InitializeCrt397,
            &pal4::CrtExitRegistration::InitializeCrt398,
            &pal4::CrtExitRegistration::InitializeCrt399,
            &pal4::CrtExitRegistration::InitializeCrt400,
            &pal4::CrtExitRegistration::InitializeCrt401,
            &pal4::CrtExitRegistration::InitializeCrt402,
            &pal4::CrtExitRegistration::InitializeCrt403,
            &pal4::CrtExitRegistration::InitializeCrt404,
            &pal4::CrtExitRegistration::InitializeCrt405,
            &pal4::CrtExitRegistration::InitializeCrt406,
            &pal4::CrtExitRegistration::InitializeCrt407,
            &pal4::CrtExitRegistration::InitializeCrt408,
            &pal4::CrtExitRegistration::InitializeCrt409,
            &pal4::CrtExitRegistration::InitializeCrt410,
            &pal4::CrtExitRegistration::InitializeCrt411,
            &pal4::CrtExitRegistration::InitializeCrt412,
            &pal4::CrtExitRegistration::InitializeCrt413,
            &pal4::CrtExitRegistration::InitializeCrt414,
            &pal4::CrtExitRegistration::InitializeCrt415,
            &pal4::CrtExitRegistration::InitializeCrt416,
            &pal4::CrtExitRegistration::InitializeCrt417,
            &pal4::CrtExitRegistration::InitializeCrt418,
            &pal4::CrtExitRegistration::InitializeCrt419,
            &pal4::CrtExitRegistration::InitializeCrt420,
            &pal4::CrtExitRegistration::InitializeCrt421,
            &pal4::CrtExitRegistration::InitializeCrt422,
            &pal4::CrtExitRegistration::InitializeCrt423,
            &pal4::CrtExitRegistration::InitializeCrt424,
            &pal4::CrtExitRegistration::InitializeCrt425,
            &pal4::CrtExitRegistration::InitializeCrt426,
            &pal4::CrtExitRegistration::InitializeCrt427,
            &pal4::CrtExitRegistration::InitializeCrt428,
            &pal4::CrtExitRegistration::InitializeCrt429,
            &pal4::CrtExitRegistration::InitializeCrt430,
            &pal4::CrtExitRegistration::InitializeCrt431,
            &pal4::CrtExitRegistration::InitializeCrt432,
            &pal4::CrtExitRegistration::InitializeCrt433,
            &pal4::CrtExitRegistration::InitializeCrt434,
            &pal4::CrtExitRegistration::InitializeCrt435,
            &pal4::CrtExitRegistration::InitializeCrt436,
            &pal4::CrtExitRegistration::InitializeCrt437,
            &pal4::CrtExitRegistration::InitializeCrt438,
            &pal4::CrtExitRegistration::InitializeCrt439,
            &pal4::CrtExitRegistration::InitializeCrt440,
            &pal4::CrtExitRegistration::InitializeCrt441,
            &pal4::CrtExitRegistration::InitializeCrt442,
            &pal4::CrtExitRegistration::InitializeCrt443,
            &pal4::CrtExitRegistration::InitializeCrt444,
            &pal4::CrtExitRegistration::InitializeCrt445,
            &pal4::CrtExitRegistration::InitializeCrt446,
            &pal4::CrtExitRegistration::InitializeCrt447,
            &pal4::CrtExitRegistration::InitializeCrt448,
            &pal4::CrtExitRegistration::InitializeCrt449,
            &pal4::CrtExitRegistration::InitializeCrt450,
            &pal4::CrtExitRegistration::InitializeCrt451,
            &pal4::CrtExitRegistration::InitializeCrt452,
            &pal4::CrtExitRegistration::InitializeCrt453,
            &pal4::CrtExitRegistration::InitializeCrt454,
            &pal4::CrtExitRegistration::InitializeCrt455,
            &pal4::CrtExitRegistration::InitializeCrt456,
            &pal4::CrtExitRegistration::InitializeCrt457,
            &pal4::CrtExitRegistration::InitializeCrt458,
            &pal4::CrtExitRegistration::InitializeCrt459,
            &pal4::CrtExitRegistration::InitializeCrt460,
            &pal4::CrtExitRegistration::InitializeCrt461,
            &pal4::CrtExitRegistration::InitializeCrt462,
            &pal4::CrtExitRegistration::InitializeCrt463,
            &pal4::CrtExitRegistration::InitializeCrt464,
            &pal4::CrtExitRegistration::InitializeCrt465,
            &pal4::CrtExitRegistration::InitializeCrt466,
            &pal4::CrtExitRegistration::InitializeCrt467,
            &pal4::CrtExitRegistration::InitializeCrt468,
            &pal4::CrtExitRegistration::InitializeCrt469,
            &pal4::CrtExitRegistration::InitializeCrt470,
            &pal4::CrtExitRegistration::InitializeCrt471,
            &pal4::CrtExitRegistration::InitializeCrt472,
            &pal4::CrtExitRegistration::InitializeCrt473,
            &pal4::CrtExitRegistration::InitializeCrt474,
            &pal4::CrtExitRegistration::InitializeCrt475,
            &pal4::CrtExitRegistration::InitializeCrt476,
            &pal4::CrtExitRegistration::InitializeCrt477,
            &pal4::CrtExitRegistration::InitializeCrt478};
        for (std::size_t index = 0; index < later_initializers.size(); ++index) {
            const auto expected = -301 - static_cast<int>(index);
            runtime.return_value = expected;
            Check(later_initializers[index](guard, runtime) == expected &&
                      guard == 0x41u &&
                      runtime.call_count == 114u + index * 2u,
                  "later CRT wrapper registers on an already set guard");
            std::uint8_t fresh_guard = 0x80u;
            Check(later_initializers[index](fresh_guard, runtime) == expected &&
                      fresh_guard == 0x81u &&
                      runtime.call_count == 115u + index * 2u,
                  "later CRT wrapper preserves unrelated guard bits");
        }
    }
    FakeAtexit offset_registration;
    offset_registration.return_value = -19;
    std::uint32_t counter = 17u;
    Check(pal4::CrtTemporaryOffset::InitializeAndRegister(
              counter, offset_registration) == -19 &&
              counter == 0u &&
              offset_registration.call_count == 1 &&
              offset_registration.last_callback != nullptr,
          "temporary offset initializer resets and registers exit callback");
    pal4::CrtTemporaryOffset::Increment(counter);
    pal4::CrtTemporaryOffset::Increment(counter);
    pal4::CrtTemporaryOffset::Decrement(counter);
    Check(counter == 1u, "temporary offset increments and decrements");
    pal4::CrtTemporaryOffset::Initialize(counter);
    pal4::CrtTemporaryOffset::Decrement(counter);
    Check(counter == 0xffffffffu,
          "temporary offset decrement uses native 32-bit wraparound");
    pal4::CrtTemporaryOffset::Increment(counter);
    offset_registration.last_callback();
    Check(counter == 0u, "registered temporary offset callback is empty");
    std::cout << "CRT atexit tests passed\n";
}
