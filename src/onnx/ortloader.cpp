#include "onnx/ortloader.h"

#include <onnxruntime_cxx_api.h>

#include <QString>
#include <QVersionNumber>
#include <string>
#include <vector>

#include "util/logger.h"

#ifdef _WIN32
#include <windows.h>
// clang-format off
#include <appmodel.h>
// clang-format on
#include <winver.h>
#else
#include <dlfcn.h>
#ifdef __linux__
#include <link.h>
#endif
#endif

namespace mixxx {
namespace onnx {

namespace {

const Logger kLogger("OrtLoader");

// Reason why the runtime cannot be used, set by the failing step
QString& failureReason() {
    static QString reason;
    return reason;
}

// Returns the platform specific file name of a shared library, e.g.
// "onnxruntime" -> "onnxruntime.dll", "libonnxruntime.so", "libonnxruntime.dylib"
QString libraryFileName(const QString& baseName) {
#if defined(_WIN32)
    return baseName + QStringLiteral(".dll");
#elif defined(__APPLE__)
    return QStringLiteral("lib") + baseName + QStringLiteral(".dylib");
#else
    return QStringLiteral("lib") + baseName + QStringLiteral(".so");
#endif
}

// Returns the absolute path of a shared library already loaded in this process,
// or an empty string if it is not loaded (or the path cannot be determined).
// The library is given by its base name without platform prefix/extension.
QString loadedLibraryPath(const QString& baseName) {
    const QString library = libraryFileName(baseName);
#ifdef _WIN32
    HMODULE module = GetModuleHandleW(library.toStdWString().c_str());
    if (!module) {
        return QString();
    }
    wchar_t buffer[32768];
    const DWORD len = GetModuleFileNameW(module, buffer, std::size(buffer));
    if (len == 0 || len >= std::size(buffer)) {
        return QString();
    }
    return QString::fromWCharArray(buffer, static_cast<int>(len));
#elif defined(__linux__)
    void* handle = dlopen(library.toUtf8().constData(), RTLD_LAZY | RTLD_NOLOAD);
    if (!handle) {
        return QString();
    }
    QString path;
    struct link_map* map = nullptr;
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) == 0 && map && map->l_name) {
        path = QString::fromUtf8(map->l_name);
    }
    dlclose(handle);
    return path;
#else
    Q_UNUSED(library);
    return QString();
#endif
}

#ifdef _WIN32

// Publisher ID of Microsoft, the last part of the package family name of the
// Windows App SDK runtime framework package, e.g.
// "Microsoft.WindowsAppRuntime.1.8_8wekyb3d8bbwe".
// Source: WINDOWSAPPSDK_RUNTIME_IDENTITY_PUBLISHERID in the Bootstrapper API,
// https://learn.microsoft.com/windows/windows-app-sdk/api/win32/_bootstrap/#constants
constexpr wchar_t kFrameworkPublisherId[] = L"8wekyb3d8bbwe";

// Version of the Windows ML import library, determined by CMake from the
// vcpkg package metadata. The loaded
// Microsoft.Windows.AI.MachineLearning.dll must not be older.
const QVersionNumber& winMlImportLibVersion() {
    static const QVersionNumber version =
            QVersionNumber::fromString(QStringLiteral(WINML_IMPORTLIB_VERSION));
    return version;
}

// Package family name of the Windows App SDK runtime framework package, which
// contains the Windows ML runtime (onnxruntime.dll,
// Microsoft.Windows.AI.MachineLearning.dll). It only contains major.minor of
// the Windows App SDK, which equal major.minor of the import library version.
std::wstring frameworkFamilyName() {
    return QStringLiteral("Microsoft.WindowsAppRuntime.%1.%2_%3")
            .arg(winMlImportLibVersion().majorVersion())
            .arg(winMlImportLibVersion().minorVersion())
            .arg(QString::fromWCharArray(kFrameworkPublisherId))
            .toStdWString();
}

// Reads the major.minor.build file version of the already resolvable DLL
QVersionNumber dllFileVersion(PCWSTR dllName) {
    HMODULE module = LoadLibraryW(dllName);
    if (!module) {
        return QVersionNumber();
    }
    wchar_t path[MAX_PATH * 4];
    const DWORD len = GetModuleFileNameW(module, path, static_cast<DWORD>(std::size(path)));
    if (len == 0 || len >= std::size(path)) {
        return QVersionNumber();
    }
    const DWORD infoSize = GetFileVersionInfoSizeW(path, nullptr);
    if (infoSize == 0) {
        return QVersionNumber();
    }
    std::vector<char> data(infoSize);
    if (!GetFileVersionInfoW(path, 0, infoSize, data.data())) {
        return QVersionNumber();
    }
    VS_FIXEDFILEINFO* fixedInfo = nullptr;
    UINT fixedLen = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&fixedInfo), &fixedLen) ||
            !fixedInfo) {
        return QVersionNumber();
    }
    return QVersionNumber(HIWORD(fixedInfo->dwFileVersionMS),
            LOWORD(fixedInfo->dwFileVersionMS),
            HIWORD(fixedInfo->dwFileVersionLS));
}

// The Windows ML DLL has to be at least as new as its import library
bool checkWinMlRuntimeVersion() {
    const QVersionNumber dllVersion = dllFileVersion(L"Microsoft.Windows.AI.MachineLearning.dll");
    if (dllVersion.isNull() || dllVersion < winMlImportLibVersion()) {
        failureReason() = QStringLiteral(
                "Microsoft.Windows.AI.MachineLearning.dll version %1 is older than its "
                "import library %2")
                                  .arg(dllVersion.toString(), winMlImportLibVersion().toString());
        kLogger.warning() << "Microsoft.Windows.AI.MachineLearning.dll version"
                          << dllVersion.toString() << "is older than its import library"
                          << winMlImportLibVersion().toString()
                          << "- Windows ML and ONNX Runtime are disabled";
        return false;
    }
    return true;
}

// The ONNX Runtime DLL has to support at least the API version of its import
// library (onnxruntime.lib / headers)
bool checkOrtRuntimeVersion() {
    const QVersionNumber dllVersion =
            QVersionNumber::fromString(QString::fromUtf8(Ort::GetVersionString()));
    const QVersionNumber importLibVersion(1, ORT_API_VERSION);
    if (dllVersion.isNull() || dllVersion < importLibVersion) {
        failureReason() = QStringLiteral(
                "onnxruntime.dll version %1 is older than its import library %2")
                                  .arg(dllVersion.toString(), importLibVersion.toString());
        kLogger.warning() << "onnxruntime.dll version" << dllVersion.toString()
                          << "is older than its import library" << importLibVersion.toString()
                          << "- Windows ML and ONNX Runtime are disabled";
        return false;
    }
    return true;
}

// Makes the ONNX Runtime library resolvable. Returns false on failure.
bool prepareRuntime() {
    // The package dependency stays in place for the lifetime of the process.
    // This uses the dynamic dependency API of the OS, so no Windows App SDK
    // bootstrapper DLL has to be shipped.
    PACKAGE_VERSION minVersion = {};
    // The framework package version scheme is <minor>000.x for 1.<minor>
    minVersion.Major = static_cast<UINT16>(winMlImportLibVersion().minorVersion() * 1000);
    PWSTR dependencyId = nullptr;
    const std::wstring familyName = frameworkFamilyName();
    HRESULT hr = TryCreatePackageDependency(
            nullptr,
            familyName.c_str(),
            minVersion,
            PackageDependencyProcessorArchitectures_None,
            PackageDependencyLifetimeKind_Process,
            nullptr,
            CreatePackageDependencyOptions_None,
            &dependencyId);
    if (FAILED(hr) || !dependencyId) {
        failureReason() = QStringLiteral(
                "the Windows App SDK runtime %1 (or newer) is not installed "
                "(TryCreatePackageDependency HRESULT 0x%2)")
                                  .arg(QString::fromStdWString(familyName))
                                  .arg(static_cast<quint32>(hr), 0, 16);
        kLogger.warning()
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
        kLogger.info() << "Using Windows App SDK runtime"
                       << QString::fromWCharArray(packageFullName);
    } else {
        failureReason() = QStringLiteral(
                "adding the Windows App SDK runtime package dependency failed "
                "(AddPackageDependency HRESULT 0x%1)")
                                  .arg(static_cast<quint32>(hr), 0, 16);
        kLogger.warning()
                << "Adding the Windows App SDK runtime package dependency failed, HRESULT"
                << Qt::hex << hr;
    }
    HeapFree(GetProcessHeap(), 0, packageFullName);
    HeapFree(GetProcessHeap(), 0, dependencyId);
    return ok;
}

#endif

} // namespace

bool initializePlatform() {
    // Thread safe, runs only once on all platforms
    static const bool initialized = [] {
#ifdef _WIN32
        if (!prepareRuntime()) {
            return false;
        }
        // Never use the runtime libraries if they are older than the import
        // libraries Mixxx was linked against.
        if (!checkWinMlRuntimeVersion()) {
            return false;
        }
#endif
        // ORT_API_MANUAL_INIT is defined: the OrtApi pointer must only be
        // fetched now, after the matching onnxruntime library can be resolved.
        // Otherwise the static initializer could load an older inbox library.
        try {
            Ort::InitApi();
        } catch (const std::exception& e) {
            failureReason() = QStringLiteral("initializing the ONNX Runtime API failed: %1")
                                      .arg(QString::fromUtf8(e.what()));
            kLogger.warning() << "Initializing the ONNX Runtime API failed:" << e.what();
            return false;
        }
#ifdef _WIN32
        return checkOrtRuntimeVersion();
#else
        return true;
#endif
    }();
    return initialized;
}

void logRuntimeDetails() {
    kLogger.info() << "ONNX Runtime headers/import library: API" << ORT_API_VERSION;
    if (!initializePlatform()) {
        kLogger.warning() << "ONNX Runtime could not be loaded:"
                          << failureReason()
                          << "All related functionality will be disabled.";
        return;
    }
    kLogger.info() << "ONNX Runtime library:" << Ort::GetVersionString().c_str()
                   << loadedLibraryPath(QStringLiteral("onnxruntime"));
#ifdef _WIN32
    kLogger.info() << "Windows ML import library:" << winMlImportLibVersion().toString();
    kLogger.info() << "Windows ML library:"
                   << loadedLibraryPath(QStringLiteral("Microsoft.Windows.AI.MachineLearning"));
    kLogger.info() << "ONNX Runtime load conditions are fulfilled: onnxruntime.dll"
                   << Ort::GetVersionString().c_str() << "is not older than its import library API"
                   << ORT_API_VERSION << "and Microsoft.Windows.AI.MachineLearning.dll"
                   << dllFileVersion(L"Microsoft.Windows.AI.MachineLearning.dll").toString()
                   << "is not older than its import library" << winMlImportLibVersion().toString();
#else
#endif
    kLogger.info() << "ONNX Runtime loaded successfully, and it's usage is enabled!";
}

} // namespace onnx
} // namespace mixxx
