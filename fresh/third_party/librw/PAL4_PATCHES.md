# PAL4 integration

Upstream commit and pristine hashes are recorded in `UPSTREAM.json`. Only the
core source/CMake subset is vendored; examples, tools sources and assets are not.
License: MIT, see `LICENSE`.

Local patches:

1. `src/uvanim.cpp`: range-check the streamed previous-frame index before
   forming a C++ pointer. Invalid values become `nil`. Original PAL4 ZJM UVA
   first frames contain an unused invalid previous value; the original SDK
   oracle still returns a one-node animation and valid matrices. PAL4's adapter
   validates topology before calling the library; later frame links must be
   valid, ordered, unbranched predecessors. No interpolation math was changed.
2. `src/engine.cpp`: optional `LIBRW_NO_IMAGE_FILE_FORMATS` suppresses automatic
   image codec registration. PAL4 enables this for its CPU-only NULL backend,
   preventing a second incompatible LodePNG implementation from entering the
   final executable. PAL4 does not load images, scenes or GPU resources through
   this library.

The portable adapter uses library interpolation/apply/matrix operations and
reduces absolute time before calling `addTime` because upstream resets to time
zero when it crosses duration. SDK reference keeps the remainder and preserves
the exact duration endpoint. Missing materials/unsupported dictionaries fail
explicitly instead of creating upstream dummy animations.

Android packages `res/raw/librw_license.txt`; R36S packaging copies `LICENSE`
into its notices directory. Include the same MIT notice when preparing a
Switch release containing this library; this change has not produced one.
