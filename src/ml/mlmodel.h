#pragma once

#include <QString>
#include <memory>

namespace mixxx {
namespace ml {

/// Platform independent interface to load an ONNX model on the best available
/// hardware (NPU > GPU > CPU).
///
/// Backends (selected at build time, see cmake/modules/MixxxOnnxRuntime.cmake):
///   Windows: the Windows ML that is built into the OS (Windows.AI.MachineLearning)
///   others:  ONNX Runtime (vcpkg or distribution package)
class MlModel {
  public:
    ~MlModel();

    /// Loads the model and logs a detailed report about the selected backend.
    /// Returns nullptr on failure.
    static std::unique_ptr<MlModel> load(const QString& modelPath);

    /// Human readable description of the selected backend, e.g.
    /// "NPU 'Intel(R) AI Boost' (vendor Intel) via Windows ML"
    const QString& backendDescription() const {
        return m_backendDescription;
    }

  private:
    struct Impl;
    MlModel(std::unique_ptr<Impl> impl, QString backendDescription);

    std::unique_ptr<Impl> m_impl;
    QString m_backendDescription;
};

} // namespace ml
} // namespace mixxx
