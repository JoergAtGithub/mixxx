#pragma once

#include <onnxruntime_cxx_api.h>

#include <QString>

namespace mixxx {
namespace onnx {

/// Result of probeExecutionProvider()
struct OrtExecutionProviderProbe {
    bool ok = false;
    /// Backend description on success, error message otherwise
    QString description;
};

/// Initializes ONNX Runtime and selects the best execution provider without
/// loading a model. Slow, call it from a worker thread. Never throws.
OrtExecutionProviderProbe probeExecutionProvider();

/// Registers the platform specific execution providers (on Windows the
/// certified vendor NPU/GPU providers from the Windows ML catalog) on env, then
/// enables the best available execution provider device (NPU > GPU > CPU) on
/// the session options and returns a human readable description of it.
QString selectExecutionProvider(Ort::Env& env, Ort::SessionOptions& options);

} // namespace onnx
} // namespace mixxx
