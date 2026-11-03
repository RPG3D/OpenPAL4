#include "pal4/crt_atexit_stub.h"

namespace pal4 {
namespace {

void NullSub82() {}

}  // namespace

int CrtExitRegistration::RegisterSharedGuardCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeFacetGuards(
    std::uint8_t& first_guard, std::uint8_t& second_guard,
    ICrtAtexitApi& api) {
    first_guard = static_cast<std::uint8_t>(first_guard | 1u);
    static_cast<void>(RegisterFacetGuardOne(api));
    second_guard = static_cast<std::uint8_t>(second_guard | 1u);
    return RegisterFacetGuardTwo(api);
}

int CrtExitRegistration::RegisterFacetGuardOne(ICrtAtexitApi& api) {
    return api.Register(&NullSub82);
}

int CrtExitRegistration::RegisterFacetGuardTwo(ICrtAtexitApi& api) {
    return api.Register(&NullSub82);
}

int CrtExitRegistration::RegisterLaterCallbackOne(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwo(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterCeguiBufferCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterRectangleCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterFontCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterTextureDictCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackThree(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackFour(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterBlurCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackFive(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackSix(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackSeven(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackEight(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackNine(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTen(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackEleven(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwelve(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaplaceCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterOldTVCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterPosterizeCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackThirteen(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterRedCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterSharpenEdgesCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterTilingCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackFourteen(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackFifteen(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackSixteen(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackSeventeen(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackEighteen(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackNineteen(
        std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwenty(
        std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentyOne(
        std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentyTwo(
        std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentyThree(
        std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentyFour(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentyFive(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentySix(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentySeven(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentyEight(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackTwentyNine(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterLaterCallbackThirty(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterBehaviorZeroCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterBehaviorFourCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterBehaviorTwoCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterBehaviorThreeCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterBehaviorPathCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterObjectExtendedCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterAfterPathCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterObjectDerivedCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterRandomMovementCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterScriptEventCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterScriptProcessorCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterScriptEventDataCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterConditionArrayCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterCollectionRecordCallback(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::RegisterNullSub(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    guard = static_cast<std::uint8_t>(guard | 1U);
    return api.Register(&NullSub82);
}

int CrtExitRegistration::InitializeCrt3(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt4(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt5(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt6(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt7(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt8(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt9(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt10(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt11(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt12(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt13(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt14(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt15(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt16(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt17(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt18(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt19(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt20(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt21(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt22(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt23(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt24(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt25(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt26(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt27(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt28(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt29(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt30(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt31(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt32(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt33(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt34(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt35(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt36(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt37(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt38(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt39(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt40(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt41(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt42(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt43(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt44(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt45(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt46(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt47(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt48(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt49(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt50(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt51(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt52(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt53(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt54(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt55(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt56(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt57(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt58(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt59(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt60(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt61(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt62(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt63(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt64(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt65(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt66(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt67(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt68(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt69(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt70(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt71(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt72(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt73(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt74(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt75(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt76(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt77(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt78(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt79(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt80(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt81(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt82(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt83(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt84(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt85(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt86(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt87(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt88(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt89(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt90(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt91(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt92(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt93(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt94(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt95(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt96(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt97(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt98(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt99(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt100(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt101(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt102(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt103(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt104(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt105(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt106(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt107(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt108(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt109(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt110(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt111(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt112(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt113(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt114(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt115(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt116(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt117(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt118(std::uint8_t& guard,
                                         ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt119(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt120(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt121(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt122(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt123(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt124(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt125(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt126(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt127(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt128(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt129(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt130(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt131(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt132(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt133(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt134(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt135(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt136(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt137(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt138(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt139(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt140(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt141(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt142(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt143(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt144(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt145(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt146(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt147(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt148(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt149(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt150(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt151(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt152(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt153(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt154(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt155(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt156(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt157(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt158(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt159(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt160(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt161(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt162(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt163(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt164(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt165(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt166(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt167(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt168(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt169(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt170(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt171(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt172(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt173(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt174(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt175(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt176(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt177(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt178(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt179(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt180(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt181(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt182(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt183(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt184(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt185(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt186(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt187(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt188(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt189(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt190(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt191(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt192(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt193(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt194(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt195(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt196(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt197(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt198(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt199(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt200(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt201(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeLocaleFour(
    std::uint8_t& first, std::uint8_t& second,
    std::uint8_t& third, std::uint8_t& fourth,
    ICrtAtexitApi& api) {
    first = static_cast<std::uint8_t>(first | 1u);
    static_cast<void>(RegisterLocaleOne(api));
    second = static_cast<std::uint8_t>(second | 1u);
    static_cast<void>(RegisterLocaleTwo(api));
    third = static_cast<std::uint8_t>(third | 1u);
    static_cast<void>(RegisterLocaleThree(api));
    fourth = static_cast<std::uint8_t>(fourth | 1u);
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::RegisterLocaleOne(ICrtAtexitApi& api) {
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::RegisterLocaleTwo(ICrtAtexitApi& api) {
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::RegisterLocaleThree(ICrtAtexitApi& api) {
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::InitializeCrt202(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt203(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt204(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt205(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt206(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt207(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt208(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt209(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt210(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt211(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt212(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt213(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt214(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt215(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt216(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt217(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt218(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt219(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt220(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt221(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt222(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt223(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt224(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt225(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt226(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt227(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt228(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt229(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt230(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt231(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt232(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt233(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt234(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt235(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt236(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt237(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt238(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt239(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt240(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt241(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt242(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt243(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt244(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt245(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt246(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt247(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt248(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt249(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt250(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt251(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt252(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt253(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt254(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt255(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt256(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt257(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt258(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt259(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt260(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt261(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt262(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt263(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt264(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt265(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt266(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt267(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt268(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt269(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt270(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt271(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt272(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt273(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt274(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt275(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt276(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt277(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt278(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt279(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt280(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt281(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt282(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt283(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt284(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt285(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt286(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt287(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt288(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt289(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt290(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt291(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt292(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt293(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt294(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt295(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt296(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt297(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt298(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt299(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt300(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt301(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt302(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt303(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt304(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt305(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt306(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt307(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt308(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt309(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt310(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt311(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt312(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt313(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt314(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt315(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt316(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt317(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt318(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt319(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt320(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt321(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt322(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt323(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt324(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt325(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt326(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt327(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt328(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt329(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt330(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt331(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt332(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt333(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt334(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt335(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt336(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt337(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt338(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt339(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt340(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt341(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt342(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt343(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt344(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt345(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt346(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt347(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt348(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt349(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt350(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt351(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt352(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt353(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt354(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt355(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt356(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt357(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt358(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt359(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt360(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt361(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt362(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt363(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt364(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt365(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt366(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt367(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt368(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt369(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt370(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt371(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt372(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt373(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt374(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt375(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt376(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt377(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt378(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt379(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt380(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt381(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt382(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt383(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt384(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt385(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt386(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt387(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt388(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt389(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt390(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt391(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt392(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt393(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt394(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt395(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt396(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt397(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt398(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt399(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt400(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt401(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt402(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt403(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt404(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt405(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt406(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt407(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt408(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt409(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt410(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt411(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt412(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt413(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt414(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt415(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt416(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt417(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt418(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt419(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt420(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt421(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt422(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt423(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt424(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt425(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt426(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt427(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt428(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt429(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt430(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt431(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt432(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt433(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt434(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt435(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt436(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt437(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt438(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt439(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt440(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt441(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt442(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt443(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt444(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt445(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt446(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt447(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt448(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt449(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt450(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt451(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt452(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt453(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt454(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt455(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt456(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt457(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt458(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt459(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt460(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt461(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt462(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt463(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt464(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt465(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt466(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt467(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt468(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt469(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt470(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt471(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt472(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt473(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt474(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt475(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt476(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt477(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeCrt478(std::uint8_t& guard,
                                          ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtExitRegistration::InitializeGameWorldGuard(
    std::uint8_t& guard, ICrtAtexitApi& api) {
    return RegisterNullSub(guard, api);
}

int CrtTemporaryOffset::InitializeAndRegister(
    std::uint32_t& counter, ICrtAtexitApi& api) {
    Initialize(counter);
    return api.Register(&NoopExit);
}

void CrtTemporaryOffset::Initialize(std::uint32_t& counter) {
    counter = 0u;
}

void CrtTemporaryOffset::NoopExit() {}

void CrtTemporaryOffset::Increment(std::uint32_t& counter) {
    ++counter;
}

void CrtTemporaryOffset::Decrement(std::uint32_t& counter) {
    --counter;
}

int CrtExitRegistration::InitializeFileStreamGuards(
    std::uint8_t& first_guard, std::uint8_t& second_guard,
    ICrtAtexitApi& api) {
    first_guard = static_cast<std::uint8_t>(first_guard | 1u);
    (void)RegisterFileStreamGuardOne(api);
    second_guard = static_cast<std::uint8_t>(second_guard | 1u);
    return RegisterFacetGuardTwo(api);
}

int CrtExitRegistration::RegisterFileStreamGuardOne(ICrtAtexitApi& api) {
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::InitializeCombatLocaleFour(
    std::uint8_t& first, std::uint8_t& second,
    std::uint8_t& third, std::uint8_t& fourth,
    ICrtAtexitApi& api) {
    first = static_cast<std::uint8_t>(first | 1u);
    static_cast<void>(RegisterCombatLocaleOne(api));
    second = static_cast<std::uint8_t>(second | 1u);
    static_cast<void>(RegisterCombatLocaleTwo(api));
    third = static_cast<std::uint8_t>(third | 1u);
    static_cast<void>(RegisterCombatLocaleThree(api));
    fourth = static_cast<std::uint8_t>(fourth | 1u);
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::RegisterCombatLocaleOne(ICrtAtexitApi& api) {
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::RegisterCombatLocaleTwo(ICrtAtexitApi& api) {
    return RegisterFacetGuardOne(api);
}

int CrtExitRegistration::RegisterCombatLocaleThree(ICrtAtexitApi& api) {
    return RegisterFacetGuardOne(api);
}

}  // namespace pal4
