#pragma once

#include <onnxruntime_cxx_api.h>

namespace mixxx {
namespace ml {

/// The only platform specific part of the ONNX Runtime integration. Must be
/// called before any ONNX Runtime API is used. On Windows the runtime DLLs ship
/// next to mixxx.exe (self-contained Windows App SDK), so nothing needs to be
/// bootstrapped. Returns false if ONNX Runtime is unusable.
bool initializePlatform();

/// Called after the environment has been created. On Windows it makes sure the
/// certified vendor execution providers (NPU/GPU) are installed via the
/// Windows ML catalog and registers them, so they show up in
/// Env::GetEpDevices(). A no-op on other platforms.
void registerPlatformExecutionProviders(Ort::Env& env);

} // namespace ml
} // namespace mixxx
