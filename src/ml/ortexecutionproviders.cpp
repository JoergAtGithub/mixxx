#include "ml/ortexecutionproviders.h"

#include <QString>
#include <string>

#include "util/logger.h"

#ifdef _WIN32
#include <windows.h>
// clang-format off
#include <appmodel.h>
// clang-format on
#include <WinMLEpCatalog.h>
#endif

namespace mixxx {
namespace ml {

#ifdef _WIN32

namespace {
// Framework package of the Windows App SDK runtime which contains the Windows
// ML runtime (onnxruntime.dll, Microsoft.Windows.AI.MachineLearning.dll)
constexpr PCWSTR kFrameworkFamilyName = L"Microsoft.WindowsAppRuntime.1.8_8wekyb3d8bbwe";
} // namespace

bool initializePlatform() {
    // Thread safe, runs only once. The package dependency stays in place for
    // the lifetime of the process. This uses the dynamic dependency API of the
    // OS, so no Windows App SDK bootstrapper DLL has to be shipped.
    static const bool initialized = [] {
        const Logger logger("OrtExecutionProviders");
        PACKAGE_VERSION minVersion = {};
        minVersion.Major = 8000;
        PWSTR dependencyId = nullptr;
        HRESULT hr = TryCreatePackageDependency(
                nullptr,
                kFrameworkFamilyName,
                minVersion,
                PackageDependencyProcessorArchitectures_None,
                PackageDependencyLifetimeKind_Process,
                nullptr,
                CreatePackageDependencyOptions_None,
                &dependencyId);
        if (FAILED(hr) || !dependencyId) {
            logger.warning()
                    << "Creating the Windows App SDK runtime package dependency failed, HRESULT"
                    << Qt::hex << hr;
            return false;
        }
        PACKAGEDEPENDENCY_CONTEXT context = nullptr;
        PWSTR packageFullName = nullptr;
        hr = AddPackageDependency(
                dependencyId,
                0,
                AddPackageDependencyOptions_None,
                &context,
                &packageFullName);
        const bool ok = SUCCEEDED(hr);
        if (ok) {
            logger.info() << "Using Windows App SDK runtime"
                          << QString::fromWCharArray(packageFullName);
        } else {
            logger.warning()
                    << "Adding the Windows App SDK runtime package dependency failed, HRESULT"
                    << Qt::hex << hr;
        }
        HeapFree(GetProcessHeap(), 0, packageFullName);
        HeapFree(GetProcessHeap(), 0, dependencyId);
        if (!ok) {
            return false;
        }
        // ORT_API_MANUAL_INIT is defined: the OrtApi pointer must only be
        // fetched now, after the matching onnxruntime.dll can be resolved.
        // Otherwise the static initializer would load an older inbox DLL.
        try {
            Ort::InitApi();
        } catch (const std::exception& e) {
            logger.warning() << "Initializing the ONNX Runtime API failed:" << e.what();
            return false;
        }
        return true;
    }();
    return initialized;
}

#else

bool initializePlatform() {
    return true;
}

#endif

#ifdef _WIN32

namespace {

const Logger kLogger("OrtExecutionProviders");

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

} // namespace ml
} // namespace mixxx
