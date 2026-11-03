#include "pal4/portable/dff_collision.h"
#include "pal4/portable/package.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
void Check(bool ok){if(!ok)throw std::runtime_error("DFF collision mismatch");}
void Compare(const std::vector<CollisionTriangle>& actual,const std::vector<CollisionTriangle>& expected){Check(actual.size()==expected.size());for(std::size_t i=0;i<actual.size();++i){const auto& a=actual[i];const auto& e=expected[i];Check(a.atomic==e.atomic && a.material==e.material && a.triangle==e.triangle && a.attributes_known==e.attributes_known && a.rgba==e.rgba);for(unsigned p=0;p<3;++p)for(unsigned d=0;d<3;++d)Check(std::abs(a.points[p][d]-e.points[p][d])<0.001f);}}
int main(int argc,char** argv){try{
    if(argc==5){std::ifstream package_file(argv[1],std::ios::binary),cache(argv[4],std::ios::binary);PackageReader package(package_file);const auto expected=ReadCollision(cache);const auto floor=package.Read(argv[2]),wall=package.Read(argv[3]);Compare(ReadDffCollision(floor),expected.floor);Compare(ReadDffCollision(wall),expected.wall);
        for(std::size_t n=0;n<floor.size();n+=floor.size()/32+1){bool failed=false;try{(void)ReadDffCollision(std::span(floor).first(n));}catch(const std::exception&){failed=true;}Check(failed);}
        std::cout<<"original DFF floor="<<expected.floor.size()<<" wall="<<expected.wall.size()<<'\n';return 0;}
    bool failed=false;try{(void)ReadDffCollision({});}catch(const std::exception&){failed=true;}Check(failed);return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
