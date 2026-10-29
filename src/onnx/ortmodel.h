#pragma once

#include <QString>
#include <memory>

namespace mixxx {
namespace onnx {

/// Platform independent interface to load an ONNX model on the best available
/// hardware (NPU > GPU > CPU).
/// Backend detection (ORT, execution providers, devices) lives in ortselectexecutionprovider.h.
class OrtModel {
  public:
    ~OrtModel();

    /// Loads the model and logs a detailed report about the selected backend.
    /// Returns nullptr on failure.
    static std::unique_ptr<OrtModel> load(const QString& modelPath);

    /// Human readable description of the selected backend, e.g.
    /// "NPU 'Intel(R) AI Boost' (vendor Intel) via Windows ML"
    const QString& backendDescription() const {
        return m_backendDescription;
    }

  private:
    struct Impl;
    OrtModel(std::unique_ptr<Impl> impl, QString backendDescription);

    std::unique_ptr<Impl> m_impl;
    QString m_backendDescription;
};

} // namespace onnx
} // namespace mixxx
