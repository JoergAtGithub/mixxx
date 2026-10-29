#include "onnx/ortmodel.h"

#include <onnxruntime_cxx_api.h>

#include <filesystem>

#include "onnx/ortloader.h"
#include "onnx/ortselectexecutionprovider.h"
#include "util/logger.h"

namespace mixxx {
namespace onnx {

namespace {
const Logger kLogger("OrtModel");
} // namespace

struct OrtModel::Impl {
    Impl(Ort::Env&& env, Ort::Session&& session)
            : env(std::move(env)),
              session(std::move(session)) {
    }
    Ort::Env env;
    Ort::Session session;
};

OrtModel::OrtModel(std::unique_ptr<Impl> impl, QString backendDescription)
        : m_impl(std::move(impl)),
          m_backendDescription(std::move(backendDescription)) {
}

OrtModel::~OrtModel() = default;

// static
std::unique_ptr<OrtModel> OrtModel::load(const QString& modelPath) {
    try {
        if (!initializePlatform()) {
            return nullptr;
        }
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "mixxx");
        Ort::SessionOptions options;
        const QString backend = selectExecutionProvider(env, options);

        const std::filesystem::path path(modelPath.toStdU16String());
        Ort::Session session(env, path.c_str(), options);

        kLogger.info() << "Model loaded:" << modelPath;
        kLogger.info() << "Selected backend:" << backend;
        return std::unique_ptr<OrtModel>(new OrtModel(
                std::make_unique<Impl>(std::move(env), std::move(session)),
                backend));
    } catch (const std::exception& e) {
        kLogger.warning() << "Failed to load ONNX model" << modelPath << ":" << e.what();
    }
    return nullptr;
}

} // namespace onnx
} // namespace mixxx
