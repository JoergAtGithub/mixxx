#pragma once

#include <onnxruntime_cxx_api.h>

namespace mixxx {
namespace ml {

/// The only platform specific part of the ONNX Runtime integration. Must be
/// called before any ONNX Runtime API is used. On Windows it initializes the
/// Windows App SDK dynamic dependency bootstrapper, so that the delay loaded
/// onnxruntime.dll and Windows ML DLL are resolved from the installed (framework
/// dependent) Windows App SDK runtime. Returns false if that fails.
bool initializePlatform();

/// Called after the environment has been created. On Windows it makes sure the
/// certified vendor execution providers (NPU/GPU) are installed via the
/// Windows ML catalog and registers them, so they show up in
/// Env::GetEpDevices(). A no-op on other platforms.
void registerPlatformExecutionProviders(Ort::Env& env);

} // namespace ml
} // namespace mixxx
