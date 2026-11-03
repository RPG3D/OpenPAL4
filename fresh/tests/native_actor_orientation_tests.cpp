#include "pal4/base_object_orientation.h"
#include "pal4/game_vector_math.h"
#include "pal4/native_legacy_crt_math.h"
#include <bit>
#include <cmath>
#include <float.h>
#include <iostream>
#include <stdexcept>

namespace {
class Runtime final : public pal4::IBaseObjectOrientationRuntime {
public:
    Runtime(){tables.InitializeConstants();(void)tables.CalculateSquareRootTable();}
    void NormalizeAndClamp(pal4::BaseObjectVector3& vector) override {
        pal4::GameVector3 value{vector.x,vector.y,vector.z};
        pal4::GameVectorMath::NormalizeAndClamp(value,tables);
        vector={value[0],value[1],value[2]};
    }
    double AngleFromCosine(double value) override {
        cosine=value;return pal4::NativeLegacyCrtMath::Acos(value);
    }
    void SetPose(pal4::BaseObjectState&,const pal4::BaseObjectVector3& value,std::uint32_t) override {yaw=value.y;}
    void SetDirection(pal4::BaseObjectState&,const pal4::BaseObjectVector3&,std::uint32_t) override {}
    pal4::GameMathTables tables;double cosine{};float yaw{};
};
}
int main(){
    const auto saved=_controlfp(0,0);
    try {
        (void)_controlfp(_PC_24,_MCW_PC);
        Runtime runtime;pal4::BaseObjectState actor;actor.Write(0x14u,1u);
        // Original pair308 held329 m15-04-08/helper6. Both helper forward
        // inputs match; original yaw1132490491, former native1132490490.
        const pal4::BaseObjectVector3 forward{std::bit_cast<float>(3212398087u),0.f,std::bit_cast<float>(3194529918u)};
        pal4::BaseObjectOrientation::Update(actor,forward,0u,0u,{},runtime);
        const auto yaw=std::bit_cast<std::uint32_t>(runtime.yaw);
        std::cout<<"captured helper6 cosine="<<runtime.cosine<<" yaw_bits="<<yaw<<" expected1132490491\n";
        if(yaw!=1132490491u)throw std::runtime_error("actor behavior orientation differs from original captured yaw");
        (void)_controlfp(saved,_MCW_PC|_MCW_RC);
        std::cout<<"PASS: original actor orientation uses public legacy acos and PC24 reflected radians\n";return 0;
    }catch(const std::exception& error){
        (void)_controlfp(saved,_MCW_PC|_MCW_RC);std::cerr<<error.what()<<'\n';return 1;
    }
}
