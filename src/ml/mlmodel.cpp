#include "ml/mlmodel.h"

#include <onnxruntime_cxx_api.h>

#include <filesystem>

#include "ml/ortbackend.h"
#include "ml/ortexecutionproviders.h"
#include "util/logger.h"

namespace mixxx {
namespace ml {

namespace {
const Logger kLogger("MlModel");
} // namespace

struct MlModel::Impl {
    Impl(Ort::Env&& env, Ort::Session&& session)
            : env(std::move(env)),
              session(std::move(session)) {
    }
    Ort::Env env;
    Ort::Session session;
};

MlModel::MlModel(std::unique_ptr<Impl> impl, QString backendDescription)
        : m_impl(std::move(impl)),
          m_backendDescription(std::move(backendDescription)) {
}

MlModel::~MlModel() = default;

// static
std::unique_ptr<MlModel> MlModel::load(const QString& modelPath) {
    try {
        if (!initializePlatform()) {
            return nullptr;
        }
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "mixxx");
        registerPlatformExecutionProviders(env);
        Ort::SessionOptions options;
        const QString backend = selectBackend(env, options);

        const std::filesystem::path path(modelPath.toStdU16String());
        Ort::Session session(env, path.c_str(), options);

        kLogger.info() << "Model loaded:" << modelPath;
        kLogger.info() << "Selected backend:" << backend;
        return std::unique_ptr<MlModel>(new MlModel(
                std::make_unique<Impl>(std::move(env), std::move(session)),
                backend));
    } catch (const std::exception& e) {
        kLogger.warning() << "Failed to load ONNX model" << modelPath << ":" << e.what();
    }
    return nullptr;
}

} // namespace ml
} // namespace mixxx
