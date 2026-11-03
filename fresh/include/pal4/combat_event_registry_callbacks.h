#pragma once

#include <cstdint>

namespace pal4 {

class CombatEventRegistryCallbacks final {
public:
    using Callback = void (*)();
    static constexpr std::uint32_t kFirstAddress = 0x583080u;
    static constexpr std::uint32_t kCount = 77u;

    [[nodiscard]] static Callback Resolve(std::uint32_t native_address);
    static void NullSub4();
    static void NullSub5();
    static void NullSub6();
    static void NullSub7();
    static void NullSub8();
    static void NullSub9();
    static void NullSub10();
    static void NullSub11();
    static void NullSub12();
    static void NullSub13();
    static void NullSub14();
    static void NullSub15();
    static void NullSub16();
    static void NullSub17();
    static void NullSub18();
    static void NullSub19();
    static void NullSub20();
    static void NullSub21();
    static void NullSub22();
    static void NullSub23();
    static void NullSub24();
    static void NullSub25();
    static void NullSub26();
    static void NullSub27();
    static void NullSub28();
    static void NullSub29();
    static void NullSub30();
    static void NullSub31();
    static void NullSub32();
    static void NullSub33();
    static void NullSub34();
    static void NullSub35();
    static void NullSub36();
    static void NullSub37();
    static void NullSub38();
    static void NullSub39();
    static void NullSub40();
    static void NullSub41();
    static void NullSub42();
    static void NullSub43();
    static void NullSub44();
    static void NullSub45();
    static void NullSub46();
    static void NullSub47();
    static void NullSub48();
    static void NullSub49();
    static void NullSub50();
    static void NullSub51();
    static void NullSub52();
    static void NullSub53();
    static void NullSub54();
    static void NullSub55();
    static void NullSub56();
    static void NullSub57();
    static void NullSub58();
    static void NullSub59();
    static void NullSub60();
    static void NullSub61();
    static void NullSub62();
    static void NullSub63();
    static void NullSub64();
    static void NullSub65();
    static void NullSub66();
    static void NullSub67();
    static void NullSub68();
    static void NullSub69();
    static void NullSub70();
    static void NullSub71();
    static void NullSub72();
    static void NullSub73();
    static void NullSub74();
    static void NullSub75();
    static void NullSub76();
    static void NullSub77();
    static void NullSub78();
    static void NullSub79();
    static void NullSub80();
};

}  // namespace pal4
