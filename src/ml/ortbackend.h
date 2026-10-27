#pragma once

#include <onnxruntime_cxx_api.h>

#include <QString>

namespace mixxx {
namespace ml {

/// Result of probeBackend()
struct MlBackendProbe {
    bool ok = false;
    /// Backend description on success, error message otherwise
    QString description;
};

/// Initializes ONNX Runtime and selects the best execution provider without
/// loading a model. Slow, call it from a worker thread. Never throws.
MlBackendProbe probeBackend();

/// Enables the best available execution provider device (NPU > GPU > CPU) on
/// the session options and returns a human readable description of it.
/// The execution providers must have been registered on env before.
QString selectBackend(Ort::Env& env, Ort::SessionOptions& options);

} // namespace ml
} // namespace mixxx
