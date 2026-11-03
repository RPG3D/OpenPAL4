#include "renderware_matrix_cases.h"
#include "pal4/camera_projection_native.h"
#include <rwcore.h>
#include <d3d9.h>
#include <windows.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <type_traits>

// The compatibility macro is shipped in rwplcore.h, but its internal prototype
// is omitted. Verified cdecl callback and setter body are recorded in IDA/COFF.
extern "C" RwBool _rwMatrixSetMultFn(rwMatrixMultFn);
namespace pal4::test {
namespace {
static_assert(std::is_same_v<decltype(&_rwMatrixDeterminant),RwReal (__cdecl *)(const RwMatrix*)>);
void Require(bool ok,const char* text) { if(!ok)throw std::runtime_error(text); }
void Near(float a,float b,const char* text,float tolerance=0.0003f) { Require(std::isfinite(a)&&std::fabs(a-b)<tolerance,text); }
RwMatrix Identity() { RwMatrix m{};RwMatrixSetIdentity(&m);return m; }
std::array<float,16> Values(const RwMatrix& m) {
    return {m.right.x,m.right.y,m.right.z,0,m.up.x,m.up.y,m.up.z,0,m.at.x,m.at.y,m.at.z,0,m.pos.x,m.pos.y,m.pos.z,1};
}
std::array<float,16> ReferenceProduct(const RwMatrix& a,const RwMatrix& b) {
    const auto x=Values(a),y=Values(b);std::array<float,16> out{};
    for(int r=0;r<4;++r)for(int c=0;c<4;++c){double v=0;for(int k=0;k<4;++k)v+=double(x[r*4+k])*y[k*4+c];out[r*4+c]=static_cast<float>(v);}
    return out;
}
void MatrixNear(const RwMatrix& m,const std::array<float,16>& expected,const char* text) {
    const auto actual=Values(m);for(size_t i=0;i<actual.size();++i)Near(actual[i],expected[i],text);
}
RwMatrix General() {
    auto m=Identity();m.right={2,1,0};m.up={0,3,1};m.at={1,0,4};m.pos={5,6,7};RwMatrixUpdate(&m);return m;
}
void Errors() {
    auto m=Identity();m.right.x=2;m.up.y=3;m.at.z=4;m.pos={1,2,3};m.flags=0x40000000;
    Near(_rwMatrixDeterminant(&m),24,"determinant");Near(_rwMatrixNormalError(&m),298,"normal squared-error sum");
    Near(_rwMatrixOrthogonalError(&m),0,"orthogonal error");Near(_rwMatrixIdentityError(&m),28,"identity includes translation");
    const RwMatrixTolerance zero{0,0,0};RwMatrixOptimize(&m,&zero);
    Require(m.flags==0x40000002,"optimize independent normal/orthogonal flags and preserved unrelated bits");
    RwMatrixTolerance defaults{};Require(RwEngineGetMatrixTolerances(&defaults),"get tolerance");
    Near(defaults.Normal,.01f,"normal tolerance");Near(defaults.Orthogonal,.01f,"orthogonal tolerance");Near(defaults.Identity,.01f,"identity tolerance");
    m=Identity();m.up.x=.05f;RwMatrixOptimize(&m,nullptr);Require((m.flags&0x20003)==0x20003,"squared tolerances may classify shear as identity");
    const auto before=Values(m);m.flags|=0x40000000;
    Require(RwMatrixUpdate(&m)==&m && m.flags==0x40000000 && Values(m)==before,"update only invalidates optimization flags");
    RwMatrixOptimize(&m,&zero);Require(!(m.flags&0x20003),"zero tolerances reject nonidentity shear");
}
int multiply_calls{};
void MultiplyProbe(RwMatrix* out,const RwMatrix* a,const RwMatrix* b) {
    ++multiply_calls;Require(a->pos.x==5 && b->pos.x==9,"multiply callback argument order");out->pos={81,82,83};
}
void Products() {
    auto a=General(),b=General();b.pos={9,10,11};RwMatrix result{};
    Require(RwMatrixMultiply(&result,&a,&b)==&result,"multiply result");MatrixNear(result,ReferenceProduct(a,b),"row-vector affine product");
    auto left=Identity();left.right.x=123;left.pad1=0x1234;left.pad2=0x5678;left.pad3=0xabcd;
    RwMatrixMultiply(&result,&left,&b);Require(std::memcmp(&result,&b,64)==0,"left identity flag trusted despite changed data");
    RwMatrixMultiply(&result,&a,&left);Require(std::memcmp(&result,&a,64)==0,"right identity flag copy");
    Require(_rwMatrixSetMultFn(MultiplyProbe),"install verified matrix callback");
    struct Reset { ~Reset(){_rwMatrixSetMultFn(nullptr);} } reset;
    a.flags=0x40000003;b.flags=0x40000001;
    Require(RwMatrixMultiply(&result,&a,&b)==&result && multiply_calls==1 && result.pos.x==81 && result.flags==0x40000001,"callback dispatch and flag intersection");
    RwMatrixMultiply(&result,&left,&b);Require(multiply_calls==1,"identity shortcut bypasses callback");
    Require(_rwMatrixSetMultFn(nullptr),"null restores scalar default");
    a.flags=0;b.flags=0;RwMatrixMultiply(&result,&a,&b);MatrixNear(result,ReferenceProduct(a,b),"scalar default after null setter");
}
void Inverses() {
    auto a=General();RwMatrix inverse{},product{};
    Require(RwMatrixInvert(&inverse,&a)==&inverse,"generic inverse result");RwMatrixMultiply(&product,&a,&inverse);MatrixNear(product,Values(Identity()),"generic inverse round trip");
    auto rigid=Identity();const RwV3d axis{0,0,1},pos{3,4,5};RwMatrixRotate(&rigid,&axis,90,rwCOMBINEREPLACE);RwMatrixTranslate(&rigid,&pos,rwCOMBINEPOSTCONCAT);
    RwMatrixInvert(&inverse,&rigid);RwMatrixMultiply(&product,&rigid,&inverse);MatrixNear(product,Values(Identity()),"orthonormal inverse round trip");
    auto bad=Identity();bad.right.x=2;bad.up.y=3;bad.at.z=4;bad.flags=3;
    RwMatrixInvert(&inverse,&bad);Near(inverse.right.x,2,"orthonormal hint trusted");Near(inverse.up.y,3,"orthonormal hint trusted y");
    bad.flags|=0x20000;RwMatrixInvert(&inverse,&bad);Require(std::memcmp(&inverse,&bad,64)==0,"identity inverse copies all bytes");
    auto singular=Identity();singular.right.x=0;singular.up.y=2;singular.at.z=3;singular.flags=0;
    Require(RwMatrixInvert(&inverse,&singular)==&inverse,"singular inverse is not failure");
    Near(inverse.right.x,6,"zero determinant uses divisor one/cofactors");Near(inverse.up.y,0,"singular cofactor y");Near(inverse.at.z,0,"singular cofactor z");
    Require(inverse.flags==0,"generic inverse flags clear");
}
void Combinations() {
    const auto original=General();const RwV3d scale{2,3,4},offset{7,8,9},axis{0,0,2};
    auto sm=Identity(),tm=Identity(),rm=Identity();RwMatrixScale(&sm,&scale,rwCOMBINEREPLACE);RwMatrixTranslate(&tm,&offset,rwCOMBINEREPLACE);RwMatrixRotate(&rm,&axis,90,rwCOMBINEREPLACE);
    Near(rm.right.x,0,"rotation degrees x");Near(rm.right.y,1,"nonunit axis normalized");
    for(auto mode:{rwCOMBINEPRECONCAT,rwCOMBINEPOSTCONCAT}) {
        auto m=original;RwMatrixScale(&m,&scale,mode);MatrixNear(m,mode==rwCOMBINEPRECONCAT?ReferenceProduct(sm,original):ReferenceProduct(original,sm),"scale combine order");
        m=original;RwMatrixTranslate(&m,&offset,mode);MatrixNear(m,mode==rwCOMBINEPRECONCAT?ReferenceProduct(tm,original):ReferenceProduct(original,tm),"translate combine order");
        m=original;RwMatrixRotate(&m,&axis,90,mode);MatrixNear(m,mode==rwCOMBINEPRECONCAT?ReferenceProduct(rm,original):ReferenceProduct(original,rm),"rotate combine order");
        m=original;RwMatrixTransform(&m,&tm,mode);MatrixNear(m,mode==rwCOMBINEPRECONCAT?ReferenceProduct(tm,original):ReferenceProduct(original,tm),"transform combine order");
    }
    auto m=original;RwMatrixTransform(&m,&tm,rwCOMBINEREPLACE);Require(std::memcmp(&m,&tm,64)==0,"replace copies complete matrix");
    m=original;Require(!RwMatrixTransform(&m,&tm,static_cast<RwOpCombineType>(99)) && std::memcmp(&m,&original,64)==0,"transform invalid mode returns null without mutation");
    const RwV3d unit{0,0,1};Require(!RwMatrixRotateOneMinusCosineSine(&m,&unit,1,1,static_cast<RwOpCombineType>(99)),"low-level rotate rejects invalid mode");
    Require(RwMatrixRotate(&m,&unit,90,static_cast<RwOpCombineType>(99))==&m && std::memcmp(&m,&original,64)==0,"Rotate wrapper ignores invalid-mode failure");
}
void NormalizeQuery() {
    auto m=General();RwMatrix result{};result.flags=0x40000000;
    Require(RwMatrixOrthoNormalize(&result,&m)==&result,"normalize thunk");
    Near(_rwMatrixNormalError(&result),0,"normalized length",.001f);Near(_rwMatrixOrthogonalError(&result),0,"normalized orthogonality",.001f);
    Near(_rwMatrixDeterminant(&result),1,"normalized handedness",.001f);Require(result.pos.x==5 && result.pos.y==6 && result.pos.z==7 && result.flags==0x40000003,"normalization position/destination flags");
    auto in_place=m;RwMatrixOrthoNormalize(&in_place,&in_place);MatrixNear(in_place,Values(result),"normalize supports alias through local copies");
    for(int axis=0;axis<3;++axis) {
        auto rotation=Identity();rotation.right.x=axis==0?1.f:-1.f;rotation.up.y=axis==1?1.f:-1.f;rotation.at.z=axis==2?1.f:-1.f;rotation.flags=3;
        RwV3d out{},center{};RwReal angle{};Require(RwMatrixQueryRotate(&rotation,&out,&angle,&center)==&rotation,"query result");
        Near(angle,180,"degenerate180 degrees");const float xyz[]{out.x,out.y,out.z};for(int i=0;i<3;++i)Near(xyz[i],i==axis?1.f:0.f,"degenerate axis largest diagonal");
    }
    auto identity=Identity();RwV3d axis{},center{};RwReal angle{};RwMatrixQueryRotate(&identity,&axis,&angle,&center);
    Require(angle==0 && axis.x==0 && axis.y==0 && axis.z==0,"identity query zero axis");
}
unsigned InvalidMode(bool scale) {
    RwMatrix m{};RwMatrixSetIdentity(&m);RwV3d v{1,2,3};
    __try { if(scale)RwMatrixScale(&m,&v,static_cast<RwOpCombineType>(99));else RwMatrixTranslate(&m,&v,static_cast<RwOpCombineType>(99));return 0; }
    __except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}
}
void InvalidDiagnostic() {
    Require(InvalidMode(true)==EXCEPTION_ACCESS_VIOLATION && InvalidMode(false)==EXCEPTION_ACCESS_VIOLATION,"invalid scale/translate must reproduce original null+12 fault");
    std::puts("Invalid matrix combine mode diagnostic: Scale and Translate fault after error path accesses null+12; valid enum precondition required.");
}
void Projection() {
    auto* scene=RwCameraCreate();auto* camera=RwCameraCreate();auto* sf=RwFrameCreate();auto* cf=RwFrameCreate();auto* parent=RwFrameCreate();
    Require(scene&&camera&&sf&&cf&&parent,"projection fixtures");
    struct Cleanup { RwCamera *a,*b;RwFrame *x,*y,*p;~Cleanup(){RwCameraDestroy(a);RwCameraDestroy(b);RwFrameRemoveChild(y);RwFrameDestroy(x);RwFrameDestroy(y);RwFrameDestroy(p);} } cleanup{scene,camera,sf,cf,parent};
    RwCameraSetFrame(scene,sf);RwCameraSetFrame(camera,cf);RwFrameAddChild(parent,cf);
    const RwV3d s{10,20,30},c{2,4,6},p{1000,2000,3000};RwFrameTranslate(sf,&s,rwCOMBINEREPLACE);RwFrameTranslate(cf,&c,rwCOMBINEREPLACE);RwFrameTranslate(parent,&p,rwCOMBINEREPLACE);
    const RwV2d window{2,3};RwCameraSetViewWindow(camera,&window);
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());Require(device!=nullptr,"projection device");
    NativeCameraProjectionRuntime runtime;CameraProjectionGlobals globals{};
    const auto set=[&]{return CameraProjectionMatrix::Set(0x1234,reinterpret_cast<std::uint32_t>(scene),reinterpret_cast<std::uint32_t>(camera),.75f,3,globals,runtime);};
    Require(set()!=0,"native game projection set");D3DMATRIX actual{};Require(SUCCEEDED(device->GetTransform(D3DTS_TEXTURE1,&actual)),"actual texture transform query");
    const float expected[]{.25f,0,0,0,0,-.25f,0,0,0,0,.2f,0,-1.5f,-3.5f,4.8f,1};
    for(int i=0;i<16;++i)Near((&actual._11)[i],expected[i],"game projection uses scene LTM but camera local modelling and x viewWindow");
    Require(globals.texture==0x1234 && globals.strength_bits==std::bit_cast<std::uint32_t>(.75f),"game projection globals");
    auto identity=Identity();RwFrameTransform(sf,&identity,rwCOMBINEREPLACE);
    Require((RwFrameGetLTM(sf)->flags&0x20000)!=0,"identity scene fixture flag");Require(set()!=0,"identity scene projection");
    Require(SUCCEEDED(device->GetTransform(D3DTS_TEXTURE1,&actual)),"identity scene query");
    Near(actual._11,-.25f,"game negates right vector without invalidating identity; multiply ignores reflection");
}
}
void RunMatrixCase(std::string_view name) {
    if(name=="matrix_errors")Errors();else if(name=="matrix_products")Products();
    else if(name=="matrix_inverse")Inverses();else if(name=="matrix_combine")Combinations();
    else if(name=="matrix_normalize_query")NormalizeQuery();else if(name=="matrix_invalid_mode")InvalidDiagnostic();
    else if(name=="matrix_game_projection")Projection();else throw std::runtime_error("unknown matrix case");
}
}
