#include <onnxruntime_cxx_api.h>

#include <filesystem>
#include <string>
#include <vector>

#include "ml/mlmodel.h"
#include "ml/mlplatform.h"
#include "util/logger.h"

namespace mixxx {
namespace ml {

namespace {

const Logger kLogger("MlModel");

#if ORT_API_VERSION >= 22
const char* deviceTypeName(OrtHardwareDeviceType type) {
    switch (type) {
    case OrtHardwareDeviceType_NPU:
        return "NPU";
    case OrtHardwareDeviceType_GPU:
        return "GPU";
    case OrtHardwareDeviceType_CPU:
        return "CPU";
    }
    return "unknown";
}

int deviceRank(OrtHardwareDeviceType type) {
    switch (type) {
    case OrtHardwareDeviceType_NPU:
        return 2;
    case OrtHardwareDeviceType_GPU:
        return 1;
    default:
        return 0;
    }
}

QString describeDevice(const Ort::ConstEpDevice& epDevice) {
    const auto hw = epDevice.Device();
    return QStringLiteral("%1 via '%2' (EP vendor: %3, hardware vendor: %4, device id: %5)")
            .arg(QLatin1String(deviceTypeName(hw.Type())),
                    QString::fromUtf8(epDevice.EpName()),
                    QString::fromUtf8(epDevice.EpVendor()),
                    QString::fromUtf8(hw.Vendor()),
                    QString::number(hw.DeviceId()));
}
#endif

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
        kLogger.info() << "ONNX Runtime version:"
                       << QString::fromUtf8(Ort::GetVersionString());

        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "mixxx");
        registerPlatformExecutionProviders(env);
        Ort::SessionOptions options;
        QString backend = QStringLiteral("CPU via 'CPUExecutionProvider' (default)");

#if ORT_API_VERSION >= 22
        const std::vector<Ort::ConstEpDevice> devices = env.GetEpDevices();
        kLogger.info() << "Available execution provider devices:" << devices.size();
        const Ort::ConstEpDevice* best = nullptr;
        for (const auto& device : devices) {
            kLogger.info() << "  -" << describeDevice(device);
            if (device.Device().Type() == OrtHardwareDeviceType_CPU) {
                continue;
            }
            if (!best ||
                    deviceRank(device.Device().Type()) >
                            deviceRank(best->Device().Type())) {
                best = &device;
            }
        }
        if (best) {
            try {
                options.AppendExecutionProvider_V2(env, {*best}, Ort::KeyValuePairs{});
                backend = describeDevice(*best) +
                        QStringLiteral(", CPU as fallback for unsupported operators");
            } catch (const Ort::Exception& e) {
                kLogger.warning() << "Failed to enable" << describeDevice(*best)
                                  << ":" << e.what() << "- falling back to CPU";
            }
        }
#else
        kLogger.info() << "ONNX Runtime older than 1.22: automatic device "
                          "selection not available, using CPU";
#endif

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
