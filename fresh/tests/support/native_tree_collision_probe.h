#pragma once
#include "pal4/native_scene_movement_asset.h"
#include "pal4/actor_control_input_component.h"
#include <bit>
#include "pal4/native_scene_player_movement.h"
#include <rwcore.h>
#include <cmath>
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
// Immutable live81 original input from trace6080 and4276B0 globals. This replay
// owns its scene assets; it never sets a live game's pose, clock or random state.
inline void VerifyNativeOriginalTreeCollision(const std::filesystem::path& root) {
    NativeSceneMovementAsset scene;
    if(!scene.Load(root,"Q01","Q01") || scene.RegisteredWallAtomics()!=1u)
        throw std::runtime_error("tree collision replay requires original Q01 wall");
    NativeScenePlayerMovement movement(scene);
    ActorControlInputComponentState input{};
    ActorControlInputComponentRuntimeStub input_runtime;
    (void)ActorControlInputComponent::Construct(input,input_runtime);
    const auto speed=std::bit_cast<float>(input.words[1]);
    struct Case { const char* name; float elapsed; CollisionPoint3 offset; bool blocked; };
    const std::array cases{
        Case{"original-blocked-17ms",0.017f,{0,0,0},true},
        Case{"original-blocked-18ms",0.018f,{0,0,0},true},
        Case{"original-previous-frame",0.017f,{-2.0343017578125f,-2.21124267578125f,2.05035400390625f},false},
        // Keep original direction/time/radius; change only to the other observed
        // position. This distinguishes boundary sensitivity from an always-block fixture.
        Case{"other-observed-position",0.017f,{1.4072265625f,0.03118896484375f,2.4632568359375f},false}
    };
    for(const auto& sample : cases) {
        const auto elapsed=sample.elapsed;
        ConfigCollisionPosition lower{452.9733581542969f,681.0171508789062f,-963.29248046875f,18.0f};
        ConfigCollisionPosition upper{452.9733581542969f,700.6414794921875f,-963.29248046875f,18.0f};
        for(unsigned i=0;i<3;++i){lower[i]+=sample.offset[i];upper[i]+=sample.offset[i];}
        RwV3d direction{0.704329788684845f,0.0f,-0.7098729610443115f};
        (void)RwV3dNormalize(&direction,&direction);
        const float distance=elapsed*speed;
        CollisionPoint3 displacement{direction.x*distance,0.0f,direction.z*distance};
        movement.ResolveCollision(lower,upper,displacement,0u);
        std::cout << "Original tree same-pose replay " << sample.name << " dt=" << elapsed << " displacement="
                  << displacement[0] << ',' << displacement[1] << ',' << displacement[2] << '\n';
        const auto magnitude=std::fabs(displacement[0])+std::fabs(displacement[1])+std::fabs(displacement[2]);
        if(!sample.blocked && (std::fabs(displacement[0]-2.0343017578125f)>0.0002f ||
                               std::fabs(displacement[2]+2.05035400390625f)>0.0002f))
            throw std::runtime_error("original previous-frame displacement differs in same-input replay");
        if(sample.blocked ? magnitude>0.001f : magnitude<0.1f)
            throw std::runtime_error("original tree blocked/clear contrast differs in native static-wall replay");
    }
}
} // namespace pal4::test
