#pragma once

namespace mixxx {
namespace onnx {

/// The platform specific part of loading the ONNX Runtime. Must be called
/// before any ONNX Runtime API is used. On Windows it adds the Windows App SDK
/// runtime package dependency, so that the delay loaded onnxruntime.dll and
/// Windows ML DLL are resolved from the installed (framework dependent)
/// runtime, and verifies that the DLLs are at least as new as the import
/// libraries Mixxx was built against. Then it initializes the ORT API.
/// Returns false if that fails.
bool initializePlatform();

/// Logs the versions of the import libraries / headers Mixxx was built against
/// and the versions and paths of the actually loaded libraries. Calls
/// initializePlatform() first, since the libraries are delay loaded.
void logRuntimeDetails();

} // namespace onnx
} // namespace mixxx
