#include "pal4/portable/effect_transform.h"
#include <cmath>
#include <stdexcept>
namespace pal4::portable {
EffectMatrix IdentityEffectMatrix(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
void ValidateEffectMatrix(const EffectMatrix& m){for(float v:m)if(!std::isfinite(v))throw std::logic_error("nonfinite effect matrix");if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)throw std::logic_error("effect matrix must be affine");}
EffectMatrix MultiplyEffectMatrices(const EffectMatrix& a,const EffectMatrix& b){ValidateEffectMatrix(a);ValidateEffectMatrix(b);EffectMatrix m{};for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)for(unsigned k=0;k<4;++k)m[col*4+row]+=a[k*4+row]*b[col*4+k];ValidateEffectMatrix(m);return m;}
}
