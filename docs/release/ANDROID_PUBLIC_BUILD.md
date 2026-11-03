# Android public candidate build

The existing `release` variant remains an optimized local test variant for
the device diagnostic workflow. It is debug-signed and debuggable. Do not
publish it as a production release.

`publicRelease` inherits the optimization settings, removes signingConfig,
and disables application/JNI debugging. Use the same external dependency
properties documented in `fresh/ports/android/README.md`, select an empty
asset directory outside the checkout, and invoke Gradle's
`assemblePublicRelease` task. Configure `pal4OutputRoot` outside the checkout.
The artifact is unsigned. No publisher key or signing password belongs in
source control; sign only after source/license and artifact review.

This variant has not been built or device-tested during the host source
audit. Publishing a usable APK still requires complete source/notice material
for the actual linked dependencies, original-data setup instructions,
artifact inspection, signing and device verification.

Local validation: Groovy parsing passed. Both offline Gradle task-discovery
attempts failed before project evaluation with `Unable to establish loopback
connection`, including a retry preferring IPv4. This is not AGP variant
validation; the variant must still be configured/built in a working Gradle
environment.
