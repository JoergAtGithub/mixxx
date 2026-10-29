#include "onnx/ortselectexecutionprovider.h"

#include <QScopeGuard>
#include <QThread>
#include <vector>

#include "onnx/ortloader.h"
#include "util/logger.h"

#ifdef _WIN32
#include <windows.h>
// clang-format off
#include <appmodel.h>
// clang-format on
#include <WinMLEpCatalog.h>
#endif

namespace mixxx {
namespace onnx {

namespace {

const Logger kLogger("OrtSelectExecutionProvider");

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

#ifdef _WIN32

BOOL CALLBACK registerProvider(WinMLEpHandle ep, const WinMLEpInfo* info, void* pContext) {
    auto* pEnv = static_cast<Ort::Env*>(pContext);
    kLogger.info() << "Execution provider package:" << info->name << info->version
                   << "certified:"
                   << (info->certification == WinMLEpCertification_Certified);
    if (info->certification != WinMLEpCertification_Certified) {
        return TRUE;
    }
    HRESULT hr = WinMLEpEnsureReady(ep);
    if (FAILED(hr)) {
        kLogger.warning() << "Execution provider" << info->name
                          << "cannot be made ready, HRESULT" << Qt::hex << hr;
        return TRUE;
    }
    size_t size = 0;
    if (FAILED(WinMLEpGetLibraryPathSize(ep, &size)) || size == 0) {
        return TRUE;
    }
    std::string path(size, '\0');
    size_t used = 0;
    if (FAILED(WinMLEpGetLibraryPath(ep, size, path.data(), &used))) {
        return TRUE;
    }
    try {
        pEnv->RegisterExecutionProviderLibrary(
                info->name, QString::fromUtf8(path.c_str()).toStdWString());
        kLogger.info() << "Registered execution provider" << info->name;
    } catch (const Ort::Exception& e) {
        kLogger.warning() << "Registering" << info->name << "failed:" << e.what();
    }
    return TRUE;
}

void registerPlatformExecutionProviders(Ort::Env& env) {
    WinMLEpCatalogHandle catalog = nullptr;
    HRESULT hr = WinMLEpCatalogCreate(&catalog);
    if (FAILED(hr)) {
        kLogger.warning() << "Windows ML EP catalog not available, HRESULT"
                          << Qt::hex << hr;
        return;
    }
    hr = WinMLEpCatalogEnumProviders(catalog, registerProvider, &env);
    if (FAILED(hr)) {
        kLogger.warning() << "Enumerating execution providers failed, HRESULT"
                          << Qt::hex << hr;
    }
    WinMLEpCatalogRelease(catalog);
}

#else

void registerPlatformExecutionProviders(Ort::Env&) {
}

#endif

} // namespace

QString selectExecutionProvider(Ort::Env& env, Ort::SessionOptions& options) {
    registerPlatformExecutionProviders(env);
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

OrtExecutionProviderProbe probeExecutionProvider() {
    // Name the (pooled) worker thread for the log, restore the name afterwards
    // because the thread is shared with other tasks.
    QThread* const pThread = QThread::currentThread();
    const QString previousName = pThread->objectName();
    pThread->setObjectName(QStringLiteral("OrtProbe"));
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
        Ort::SessionOptions options;
        const QString backend = selectExecutionProvider(env, options);
        kLogger.info() << "Selected backend:" << backend;
        return {true, backend};
    } catch (const std::exception& e) {
        return {false, QString::fromUtf8(e.what())};
    }
}

} // namespace onnx
} // namespace mixxx
