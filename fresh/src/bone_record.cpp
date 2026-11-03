#include "pal4/bone_record.h"

namespace pal4 {

BoneRecordState& BoneRecord::Initialize(BoneRecordState& state) {
    state.words.fill(0u);
    return state;
}

}  // namespace pal4
