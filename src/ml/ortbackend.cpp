#include "ml/ortbackend.h"

#include <QScopeGuard>
#include <QThread>
#include <vector>

#include "ml/ortexecutionproviders.h"
#include "util/logger.h"

namespace mixxx {
namespace ml {

namespace {

const Logger kLogger("OrtBackend");

static_assert(ORT_API_VERSION >= 23, "ONNX Runtime 1.23 or newer is required");

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

} // namespace

QString selectBackend(Ort::Env& env, Ort::SessionOptions& options) {
    QString backend = QStringLiteral("CPU via 'CPUExecutionProvider' (default)");
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
    return backend;
}

MlBackendProbe probeBackend() {
    // Name the (pooled) worker thread for the log, restore the name afterwards
    // because the thread is shared with other tasks.
    QThread* const pThread = QThread::currentThread();
    const QString previousName = pThread->objectName();
    pThread->setObjectName(QStringLiteral("MlProbe"));
    const auto restoreName = qScopeGuard([&] { pThread->setObjectName(previousName); });
    try {
        if (!initializePlatform()) {
            return {false, QStringLiteral("ONNX Runtime platform initialization failed")};
        }
        kLogger.info() << "ONNX Runtime version:"
                       << QString::fromUtf8(Ort::GetVersionString());
        kLogger.info() << "ONNX Runtime build info:"
                       << QString::fromUtf8(Ort::GetBuildInfoString());
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "mixxx");
        registerPlatformExecutionProviders(env);
        Ort::SessionOptions options;
        const QString backend = selectBackend(env, options);
        kLogger.info() << "Selected backend:" << backend;
        return {true, backend};
    } catch (const std::exception& e) {
        return {false, QString::fromUtf8(e.what())};
    }
}

} // namespace ml
} // namespace mixxx
