#pragma once

#include <cstdint>

namespace pal4 {

using CrtExitCallback = void (*)();

class ICrtAtexitApi {
public:
    virtual ~ICrtAtexitApi() = default;
    [[nodiscard]] virtual int Register(CrtExitCallback callback) = 0;
};

class CrtAtexitApi final : public ICrtAtexitApi {
public:
    [[nodiscard]] int Register(CrtExitCallback callback) override;
};

// Source compatibility for the former no-op implementation. Registration now
// reaches the host CRT; tests should inject their own ICrtAtexitApi fake.
using CrtAtexitStub = CrtAtexitApi;

class CrtExitRegistration final {
public:
    static constexpr std::uint32_t kInitializeCrt3GuardAddress = 0x978b3cu;
    static constexpr std::uint32_t kFacetGuardOneAddress = 0x978b3cu;
    static constexpr std::uint32_t kFacetGuardTwoAddress = 0x978b30u;

    // IDA 0x65b580: set shared guard bit and register nullsub_82 every time.
    [[nodiscard]] static int RegisterSharedGuardCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);

    [[nodiscard]] static int InitializeFacetGuards(
        std::uint8_t& first_guard, std::uint8_t& second_guard,
        ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterFacetGuardOne(ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterFacetGuardTwo(ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackOne(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwo(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterCeguiBufferCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterRectangleCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterFontCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterTextureDictCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackThree(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackFour(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterBlurCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackFive(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackSix(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackSeven(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackEight(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackNine(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackEleven(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwelve(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaplaceCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterOldTVCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterPosterizeCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackThirteen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterRedCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterSharpenEdgesCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterTilingCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackFourteen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackFifteen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackSixteen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackSeventeen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackEighteen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackNineteen(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwenty(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentyOne(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentyTwo(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentyThree(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentyFour(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentyFive(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentySix(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentySeven(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentyEight(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackTwentyNine(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLaterCallbackThirty(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterBehaviorZeroCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterBehaviorFourCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterBehaviorTwoCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterBehaviorThreeCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterBehaviorPathCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterObjectExtendedCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterAfterPathCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterObjectDerivedCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterRandomMovementCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterScriptEventCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterScriptProcessorCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterScriptEventDataCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterConditionArrayCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterCollectionRecordCallback(
        std::uint8_t& guard, ICrtAtexitApi& api);

    [[nodiscard]] static int RegisterNullSub(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt3(std::uint8_t& guard,
                                            ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt4(std::uint8_t& guard,
                                            ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt5(std::uint8_t& guard,
                                            ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt6(std::uint8_t& guard,
                                            ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt7(std::uint8_t& guard,
                                            ICrtAtexitApi& api);
    // IDA 0x409600: shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt8(std::uint8_t& guard,
                                            ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt9(std::uint8_t& guard,
                                            ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt10(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt11(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt12(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt13(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt14(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt15(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt16(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt17(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt18(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt19(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt20(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt21(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt22(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt23(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt24(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt25(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt26(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt27(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt28(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt29(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt30(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt31(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt32(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4335f0: shared guard 0x978b3c and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt33(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x435490: another shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt34(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x437740: same shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt35(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x438ff0: another registration at shared guard 0x978b3c.
    [[nodiscard]] static int InitializeCrt36(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x439920: shared guard 0x978b3c and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt37(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x43c2a0: shared guard 0x978b3c and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt38(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x43de30: shared guard 0x978b3c and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt39(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x43e650: shared guard 0x978b3c and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt40(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x440e20: shared guard 0x978b3c and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt41(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x442270: same shared guard and null callback registration.
    [[nodiscard]] static int InitializeCrt42(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x444470 and 0x4444c0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt43(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt44(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x444560: another shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt45(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x444a90: shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt46(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x445020: shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt47(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x445310: shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt48(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x445eb0: shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt49(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x447370: shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt50(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x448300 and 0x448330: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt51(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt52(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x449280: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt53(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4499e0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt54(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x44a200: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt55(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x44b000: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt56(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x44cc10: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt57(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x44daa0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt58(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x44ed50: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt59(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x47d160: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt60(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x47dc30: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt61(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x482500: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt62(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x483550: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt63(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x483b20: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt64(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x485570: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt65(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x486df0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt66(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x48dea0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt67(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4a8f80: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt68(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4a8fd0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt69(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4a9fa0/0x4a9fd0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt70(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt71(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4aa080: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt72(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4ab6c0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt73(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4aeb80: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt74(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4af730: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt75(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4b34c0: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt76(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4b3ac0: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt77(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4b5280: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt78(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4b5510: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt79(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4b6e30: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt80(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4b9210: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt81(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4bae60/0x4bae90: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt82(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt83(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4bb5f0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt84(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c2240: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt85(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c2cf0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt86(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c3310: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt87(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c4180/0x4c42b0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt88(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt89(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c5bc0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt90(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c5e50: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt91(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c6570 / 0x4c65a0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt92(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt93(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c6d70 / 0x4c6e40: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt94(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt95(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c7ff0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt96(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4c8a20: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt97(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4caf30: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt98(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4cbe90: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt99(std::uint8_t& guard,
                                             ICrtAtexitApi& api);
    // IDA 0x4ce840: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt100(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x4cf310: another registration on the same shared guard.
    [[nodiscard]] static int InitializeCrt101(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x4d1ce0: same guard and callback, registered on every call.
    [[nodiscard]] static int InitializeCrt102(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x4d51e0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt103(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x4d8000: another shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt104(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt105(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt106(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4dcfb0: same guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt107(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4dea60: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt108(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4e0740: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt109(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4e2850: another shared-guard nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt110(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4e44e0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt111(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4e5560: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt112(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4e7270: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt113(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4e9900: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt114(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4edf20: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt115(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4eee60: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt116(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4f21f0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt117(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x4f3840: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt118(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x50b110: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt119(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x50cda0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt120(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x50f900: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt121(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x510260: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt122(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x514a30: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt123(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x51b730: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt124(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x51c4d0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt125(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x51d4e0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt126(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x51db50: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt127(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x51e4d0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt128(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x51ef60: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt129(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x520c90: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt130(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x5226b0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt131(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x522b90: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt132(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x527980: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt133(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x529590: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt134(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x529710/0x529740: the same shared guard registration.
    [[nodiscard]] static int InitializeCrt135(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt136(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x529c40: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt137(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x52aed0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt138(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x5300b0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt139(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x530890: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt140(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x533350/0x533390: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt141(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt142(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x533f10: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt143(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x53af80/0x53afc0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt144(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt145(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x53b850: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt146(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x53c8c0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt147(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x53dcc0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt148(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x53e280: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt149(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x53f0e0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt150(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x53fab0/0x53fae0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt151(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt152(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x540f20: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt153(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x542b20: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt154(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x542cb0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt155(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x549e30: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt156(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x54afd0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt157(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt158(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt159(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt160(std::uint8_t& guard,
                                               ICrtAtexitApi& api);
    // IDA 0x552160: guard byte_978B3C and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt161(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt162(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt163(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt164(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt165(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt166(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt167(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt168(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt169(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x55b7f0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt170(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x55ce50: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt171(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x55f730: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt172(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x562520/0x562550: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt173(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt174(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5628f0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt175(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x562d40/0x562da0/0x562e30/0x562e60.
    [[nodiscard]] static int InitializeCrt176(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt177(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt178(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt179(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x562f60/0x562f90: event-router registration stubs.
    [[nodiscard]] static int InitializeCrt180(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt181(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x563760/0x563790/0x563a60.
    [[nodiscard]] static int InitializeCrt182(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt183(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt184(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5659d0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt185(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x566180: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt186(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x566780: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt187(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x568cc0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt188(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x568e00: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt189(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x569400: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt190(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5699a0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt191(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56a6e0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt192(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56b130: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt193(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56bd00: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt194(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56bf50: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt195(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56cda0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt196(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56d980: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt197(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56da60: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt198(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56dcd0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt199(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x56eb10/0x56ec20: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt200(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt201(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    static constexpr std::uint32_t kLocaleGuardThreeAddress = 0x978a96u;
    static constexpr std::uint32_t kLocaleGuardFourAddress = 0x978a97u;
    // IDA 0x571000/0x571060/0x571070/0x571080.
    [[nodiscard]] static int InitializeLocaleFour(
        std::uint8_t& first, std::uint8_t& second,
        std::uint8_t& third, std::uint8_t& fourth,
        ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLocaleOne(ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLocaleTwo(ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterLocaleThree(ICrtAtexitApi& api);
    // IDA 0x574130: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt202(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x574dc0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt203(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x576cb0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt204(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x579a90: guard byte 0x978b3c, bit 1.
    [[nodiscard]] static int InitializeCrt205(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x57a170: another guard at 0x978b3c, bit 1.
    [[nodiscard]] static int InitializeCrt206(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x57bed0: guard byte 0x978b3c, bit 0.
    [[nodiscard]] static int InitializeCrt207(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x57c050/0x57c080: same guard byte and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt208(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt209(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x57ce10/0x57cf40: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt210(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt211(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x57de30: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt212(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5814d0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt213(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x583550: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt214(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x58f390: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt215(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x58f900: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt216(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x58fd00: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt217(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x58fe10/0x58ff00: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt218(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt219(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5900e0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt220(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x590270: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt221(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x590430: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt222(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5905b0/0x590720: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt223(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt224(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5925a0/0x5925d0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt225(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt226(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x593200/0x593230: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt227(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt228(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x593b00: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt229(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x593f90: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt230(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x594600: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt231(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x594920: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt232(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x594d40: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt233(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x595120: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt234(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5959f0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt235(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x595fa0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt236(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x596050: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt237(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x596a20: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt238(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5975e0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt239(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x598140: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt240(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x598220: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt241(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x598350: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt242(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x598e00: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt243(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x599440: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt244(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x599870: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt245(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x599c70: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt246(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x599e70: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt247(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x599f00: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt248(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x59a9c0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt249(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x59b730: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt250(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x59c420: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt251(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x59d0d0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt252(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x59e670: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt253(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x59f350: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt254(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x59fbc0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt255(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a3c90: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt256(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a42b0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt257(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a4c20: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt258(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a5f00: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt259(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a6ca0/0x5a6cd0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt260(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt261(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a9290/0x5a9340/0x5a93a0: same shared guard.
    [[nodiscard]] static int InitializeCrt262(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt263(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt264(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a9440/0x5a94e0: same shared guard.
    [[nodiscard]] static int InitializeCrt265(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt266(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a9b40/0x5a9b80/0x5a9c70: same shared guard.
    [[nodiscard]] static int InitializeCrt267(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt268(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt269(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5a9ea0/0x5aa110: same shared guard.
    [[nodiscard]] static int InitializeCrt270(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt271(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5aa770: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt272(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5aad80: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt273(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ab2c0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt274(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5acf20/0x5ad130: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt275(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt276(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5b4540: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt277(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5b4e90/0x5b4ec0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt278(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt279(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5b5ac0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt280(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5b7400: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt281(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5bede0: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt282(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5c4400: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt283(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5c8470: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt284(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ccd80: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt285(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5cf580: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt286(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5d8c00: same shared guard at 0x978b3c.
    [[nodiscard]] static int InitializeCrt287(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5e8f90: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt288(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5eaa40: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt289(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5eacb0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt290(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ead80: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt291(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5eadb0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt292(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ec290/0x5ec2c0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt293(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt294(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ec5e0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt295(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ec710: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt296(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ec740/0x5ec770: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt297(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt298(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ec830: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt299(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ec950: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt300(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5eca10/70/a0/d0 and 0x5ecb00/30: same shared guard/callback.
    [[nodiscard]] static int InitializeCrt301(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt302(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt303(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt304(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt305(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt306(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ecd50: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt307(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ed030/0x5ed060/0x5ed090: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt308(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt309(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt310(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ed470/0x5ed4a0: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt311(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt312(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ed4f0/0x5ed520: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt313(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt314(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ee0c0: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt315(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5eecd0: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt316(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5efa90: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt317(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f01d0: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt318(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f03b0: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt319(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f1a60/0x5f1a90: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt320(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt321(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f37f0: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt322(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f5000: same shared guard and callback.
    [[nodiscard]] static int InitializeCrt323(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f7130 and 0x5f7280: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt324(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt325(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f9900: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt326(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f9d90/0x5f9dc0: same guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt327(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt328(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5f9f00: same guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt329(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fa000: same guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt330(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fa3a0: same guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt331(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fa5b0/0x5fa5e0: same guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt332(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt333(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fac30/0x5fac60: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt334(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt335(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fb0f0/0x5fb120: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt336(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt337(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fb7b0/0x5fb7e0: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt338(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt339(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fba10/0x5fba40: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt340(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt341(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fbf50: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt342(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fc040/0x5fc070: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt343(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt344(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fc1d0/0x5fc200/0x5fc300: shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt345(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt346(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt347(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fc420/0x5fc450/0x5fc480: same guard and callback.
    [[nodiscard]] static int InitializeCrt348(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt349(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt350(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fc4b0: same guard and callback.
    [[nodiscard]] static int InitializeCrt351(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fc7b0: same guard and callback.
    [[nodiscard]] static int InitializeCrt352(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fd100: same guard and callback.
    [[nodiscard]] static int InitializeCrt353(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fd660: same guard and callback.
    [[nodiscard]] static int InitializeCrt354(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5fe690: same guard and callback.
    [[nodiscard]] static int InitializeCrt355(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5feda0: same guard and callback.
    [[nodiscard]] static int InitializeCrt356(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x5ff9b0: same guard and callback.
    [[nodiscard]] static int InitializeCrt357(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6000e0: same guard and callback.
    [[nodiscard]] static int InitializeCrt358(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x601e50: same guard and callback.
    [[nodiscard]] static int InitializeCrt359(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x602be0: same guard and callback.
    [[nodiscard]] static int InitializeCrt360(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x603660: same guard and callback.
    [[nodiscard]] static int InitializeCrt361(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6042c0: same guard and callback.
    [[nodiscard]] static int InitializeCrt362(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x604550: same guard and callback.
    [[nodiscard]] static int InitializeCrt363(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x604da0: same guard and callback.
    [[nodiscard]] static int InitializeCrt364(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x606910: same guard and callback.
    [[nodiscard]] static int InitializeCrt365(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x608df0: same guard and callback.
    [[nodiscard]] static int InitializeCrt366(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x60be30: same guard and callback.
    [[nodiscard]] static int InitializeCrt367(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x60e030: same guard and callback.
    [[nodiscard]] static int InitializeCrt368(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x60f950: same guard and callback.
    [[nodiscard]] static int InitializeCrt369(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x611450: same guard and callback.
    [[nodiscard]] static int InitializeCrt370(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x614fb0: same guard and callback.
    [[nodiscard]] static int InitializeCrt371(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x616a70: same guard and callback.
    [[nodiscard]] static int InitializeCrt372(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x617d80: same guard and callback.
    [[nodiscard]] static int InitializeCrt373(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61aa70: same guard and callback.
    [[nodiscard]] static int InitializeCrt374(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61cc60: same guard and callback.
    [[nodiscard]] static int InitializeCrt375(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61dd80: same guard and callback.
    [[nodiscard]] static int InitializeCrt376(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61e320: same guard and callback.
    [[nodiscard]] static int InitializeCrt377(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61e6d0: same guard and callback.
    [[nodiscard]] static int InitializeCrt378(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61ecc0: same guard and callback.
    [[nodiscard]] static int InitializeCrt379(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61f930: same guard and callback.
    [[nodiscard]] static int InitializeCrt380(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x61fcd0: same guard and callback.
    [[nodiscard]] static int InitializeCrt381(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x620160: same guard and callback.
    [[nodiscard]] static int InitializeCrt382(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6205b0/0x6205e0: same guard and callback.
    [[nodiscard]] static int InitializeCrt383(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt384(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6210c0: same guard and callback.
    [[nodiscard]] static int InitializeCrt385(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x621460: same guard and callback.
    [[nodiscard]] static int InitializeCrt386(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x622070: same guard and callback.
    [[nodiscard]] static int InitializeCrt387(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6231c0: same guard and callback.
    [[nodiscard]] static int InitializeCrt388(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x623b90: same guard and callback.
    [[nodiscard]] static int InitializeCrt389(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x623f30: same guard and callback.
    [[nodiscard]] static int InitializeCrt390(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x624360: same guard and callback.
    [[nodiscard]] static int InitializeCrt391(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x624700: same guard and callback.
    [[nodiscard]] static int InitializeCrt392(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x624ac0: same guard and callback.
    [[nodiscard]] static int InitializeCrt393(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x624f50: same guard and callback.
    [[nodiscard]] static int InitializeCrt394(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6252f0: same guard and callback.
    [[nodiscard]] static int InitializeCrt395(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x626490: same guard and callback.
    [[nodiscard]] static int InitializeCrt396(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6275b0: same guard and callback.
    [[nodiscard]] static int InitializeCrt397(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x627b80: same guard and callback.
    [[nodiscard]] static int InitializeCrt398(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x628120: same guard and callback.
    [[nodiscard]] static int InitializeCrt399(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x628630, 0x628660 and 0x628690: same guard and callback.
    [[nodiscard]] static int InitializeCrt400(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt401(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt402(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x62cc00: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt403(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x62f040: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt404(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x62f520: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt405(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x635ce0: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt406(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x638b60: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt407(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x63ad70/0x63afc0: shared bit-zero guard and null callback.
    [[nodiscard]] static int InitializeCrt408(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt409(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x63cc50: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt410(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x63dbb0: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt411(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x63f310: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt412(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x63fbd0..0x63fc60: four identical shared-guard wrappers.
    [[nodiscard]] static int InitializeCrt413(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt414(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt415(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt416(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6421e0: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt417(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x642840..0x6428a0: three identical shared-guard wrappers.
    [[nodiscard]] static int InitializeCrt418(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt419(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt420(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6438e0/0x643910: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt421(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt422(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x644bd0/0x644c00: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt423(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt424(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x648070: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt425(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x648960/0x648990: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt426(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt427(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x649270: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt428(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x64a230: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt429(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x64a260: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt430(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x64b4a0: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt431(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x64bb80/0x64bbb0/0x64bbe0: same shared guard and null callback.
    [[nodiscard]] static int InitializeCrt432(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt433(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt434(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x64bc40/0x64bcd0/0x64bd00: same guard and null callback.
    [[nodiscard]] static int InitializeCrt435(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt436(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt437(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x64e9e0: shared guard 0x978b3c and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt438(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x64fb50: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt439(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x651b40: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt440(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x653680/0x6536d0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt441(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt442(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x653fe0: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt443(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6545d0/0x654600: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt444(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt445(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x655880: same shared guard and nullsub_82 callback.
    [[nodiscard]] static int InitializeCrt446(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6560c0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt447(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x657fc0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt448(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65a0c0/0x65a100: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt449(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt450(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65a310: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt451(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65c6a0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt452(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65d270/0x65d2a0: repeated shared-guard registrations.
    [[nodiscard]] static int InitializeCrt453(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt454(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65e200: same shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt455(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65e380: repeated nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt456(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65ebe0: repeated nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt457(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65ef30: repeated nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt458(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x65f8a0/0x65f8d0/0x65f900: repeated shared-guard wrappers.
    [[nodiscard]] static int InitializeCrt459(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt460(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    [[nodiscard]] static int InitializeCrt461(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x662ee0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt462(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x663f30: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt463(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x6655e0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt464(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x667f30: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt465(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66a880: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt466(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66aaf0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt467(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66ac00: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt468(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66b680: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt469(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66b760: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt470(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66c540: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt471(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66ca70: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt472(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66cb60: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt473(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66cef0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt474(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66d750: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt475(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66dfb0: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt476(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x66e320: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt477(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x672830: shared guard and nullsub_82 registration.
    [[nodiscard]] static int InitializeCrt478(std::uint8_t& guard,
                                              ICrtAtexitApi& api);
    // IDA 0x661910: shared guard and nullsub_82 atexit registration.
    [[nodiscard]] static int InitializeGameWorldGuard(
        std::uint8_t& guard, ICrtAtexitApi& api);
    // IDA 0x5f8960 / 0x5f8990; 0x5f89a0 is the first entry's tail chunk.
    [[nodiscard]] static int InitializeFileStreamGuards(
        std::uint8_t& first_guard, std::uint8_t& second_guard,
        ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterFileStreamGuardOne(ICrtAtexitApi& api);
    // IDA 0x57fcc0/0x57fd20/0x57fd30/0x57fd40.
    [[nodiscard]] static int InitializeCombatLocaleFour(
        std::uint8_t& first, std::uint8_t& second,
        std::uint8_t& third, std::uint8_t& fourth,
        ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterCombatLocaleOne(ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterCombatLocaleTwo(ICrtAtexitApi& api);
    [[nodiscard]] static int RegisterCombatLocaleThree(ICrtAtexitApi& api);
};

class CrtTemporaryOffset final {
public:
    static constexpr std::uint32_t kAddress = 0x93cee0u;
    // IDA 0x654630: reset counter and register nullsub_99.
    [[nodiscard]] static int InitializeAndRegister(
        std::uint32_t& counter, ICrtAtexitApi& api);
    // IDA 0x654640/0x654660: reset counter and empty exit callback.
    static void Initialize(std::uint32_t& counter);
    static void NoopExit();
    // IDA 0x654670/0x654680: increment and decrement the counter.
    static void Increment(std::uint32_t& counter);
    static void Decrement(std::uint32_t& counter);
};

}  // namespace pal4
