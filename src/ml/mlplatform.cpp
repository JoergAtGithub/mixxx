#include "ml/mlplatform.h"

#include <QString>
#include <string>

#include "util/logger.h"

#ifdef _WIN32
#include <MddBootstrap.h>
#include <WinMLEpCatalog.h>
#include <windows.h>
#endif

namespace mixxx {
namespace ml {

#ifdef _WIN32

namespace {

const Logger kLogger("MlPlatform");

// Windows App SDK 1.8
constexpr UINT32 kMajorMinor = 0x00010008;

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

} // namespace

bool initializePlatform() {
    const HRESULT hr = MddBootstrapInitialize2(kMajorMinor,
            L"",
            PACKAGE_VERSION{},
            MddBootstrapInitializeOptions_OnNoMatch_ShowUI);
    if (FAILED(hr)) {
        kLogger.warning() << "Windows App Runtime 1.8 is not available, HRESULT"
                          << Qt::hex << hr;
        return false;
    }
    return true;
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

bool initializePlatform() {
    return true;
}

void registerPlatformExecutionProviders(Ort::Env&) {
}

#endif

} // namespace ml
} // namespace mixxx
