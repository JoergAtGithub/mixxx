#pragma once

#include <onnxruntime_cxx_api.h>

namespace mixxx {
namespace ml {

/// The only platform specific part. Must be called before any ONNX Runtime
/// API is used. On Windows it bootstraps the Windows App Runtime (the ONNX
/// Runtime is delay loaded from that framework package). Returns false if
/// ONNX Runtime is unusable.
bool initializePlatform();

/// Called after the environment has been created. On Windows it makes sure the
/// certified vendor execution providers (NPU/GPU) are installed and registers
/// them, so they show up in Env::GetEpDevices().
void registerPlatformExecutionProviders(Ort::Env& env);

} // namespace ml
} // namespace mixxx
