#include "pal4/portable/movement.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool b) { if (!b) throw std::runtime_error("movement contract failed"); }
CollisionMesh Fixture() {
    CollisionMesh mesh;
    mesh.floor={{{Point3{-100,0,-100},Point3{100,0,-100},Point3{-100,0,100}}},
                {{Point3{100,0,-100},Point3{100,0,100},Point3{-100,0,100}}}};
    mesh.wall={{{Point3{-100,-100,0},Point3{100,-100,0},Point3{-100,100,0}}},
               {{Point3{100,-100,0},Point3{100,100,0},Point3{-100,100,0}}}};
    return mesh;
}
int main(int argc,char** argv) {
    try {
        MovementConfig config; config.height=20; config.speed=100;
        MovementState state; state.position={0,0,-50}; const auto mesh=Fixture();
        StepActorMovement(state,config,mesh,{1,0,0},.05f);
        Check(std::abs(state.position[2]+45)<1e-5f && std::abs(state.yaw-90)<1e-4f && state.animation=="C02");
        StepActorMovement(state,config,mesh,{1,0,0},.05f);
        Check(std::abs(state.position[0]-5)<1e-4f);
        StepActorMovement(state,config,mesh,{0,0,0},.05f); Check(state.animation=="C01" && !state.input_active);
        state={}; state.position={0,0,-50};
        for (int i=0;i<30;++i) StepActorMovement(state,config,mesh,{0,0,1},.05f);
        Check(state.clipped_steps>0 && state.position[2]<-10 && state.distance>0 && state.last_obstacle==MovementObstacle::wall);
        const auto wall_z=state.position[2];
        for (int i=0;i<3;++i) StepActorMovement(state,config,mesh,{0,0,-1},.05f);
        Check(state.position[2]<wall_z); // blocked movement must not trap heading
        state={}; state.position={200,0,200};
        StepActorMovement(state,config,mesh,{0,0,1},.05f);
        Check(state.rejected_steps==1 && state.position[0]==200 && state.yaw==0 && !state.floor);
        bool bad=false; try { StepActorMovement(state,config,mesh,{0,0,1},1); } catch (const std::runtime_error&) { bad=true; } Check(bad);
        if (argc==3) {
            std::ifstream c(argv[1],std::ios::binary),l(argv[2],std::ios::binary);
            const auto collision=ReadCollision(c); const auto level=ReadLevel(l);
            MovementState real; real.position=level.position; real.yaw=level.yaw;
            MovementConfig settings;
            for (int i=0;i<300;++i) StepActorMovement(real,settings,collision,{0,0,-1},1.f/60);
            Check(real.accepted_steps>0 && real.clipped_steps>0 && real.distance>1 && real.floor.has_value());
            std::cout<<"N01 accepted="<<real.accepted_steps<<" clipped="<<real.clipped_steps<<" distance="<<real.distance
                     <<" position="<<real.position[0]<<','<<real.position[1]<<','<<real.position[2]<<'\n';
        }
        std::cout<<"movement source heading, clips, wall stop/escape, missing floor and timestep contracts passed\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
