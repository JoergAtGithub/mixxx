#include "controllers/hid/hiddevice.h"

#include <hidapi.h>

#include <QDebugStateSaver>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

#include "controllers/controllermappinginfo.h"
#include "util/string.h"

namespace {

constexpr unsigned short kGenericDesktopPointerUsage = 0x01;
constexpr unsigned short kGenericDesktopJoystickUsage = 0x04;
constexpr unsigned short kGenericDesktopGamePadUsage = 0x05;
constexpr unsigned short kGenericDesktopKeypadUsage = 0x07;
constexpr unsigned short kGenericDesktopMultiaxisControllerUsage = 0x08;

constexpr unsigned short kAppleInfraredControlProductId = 0x8242;

constexpr std::size_t kDeviceInfoStringMaxLength = 512;

// The HID Usage Tables 1.5 PDF specifies that the vendor-defined Usage-Page
// range is 0xFF00 to 0xFFFF.
constexpr uint16_t kStartOfVendorDefinedUsagePageRange = 0xFF00;

} // namespace

namespace mixxx {

namespace hid {

HidUsageTables::HidUsageTables(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open HID usage tables file:" << filePath;
        m_hidUsageTables = QJsonObject();
        return;
    }
    QByteArray fileData = file.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData);
    m_hidUsageTables = jsonDoc.object();
}

QString HidUsageTables::getUsagePageDescription(unsigned short usagePage) const {
    if (usagePage >= kStartOfVendorDefinedUsagePageRange) {
        return QStringLiteral("Vendor-defined");
    }

    const QJsonArray usagePages = m_hidUsageTables.value("UsagePages").toArray();
    for (const QJsonValue& pageValue : usagePages) {
        QJsonObject pageObject = pageValue.toObject();
        if (pageObject.value("Id").toInt() == usagePage) {
            return pageObject.value("Name").toString();
        }
    }
    return QStringLiteral("Reserved");
}

QString HidUsageTables::getUsageDescription(unsigned short usagePage, unsigned short usage) const {
    if (usagePage >= kStartOfVendorDefinedUsagePageRange) {
        return QStringLiteral("Vendor-defined");
    }

    const QJsonArray usagePages = m_hidUsageTables.value("UsagePages").toArray();
    for (const QJsonValue& pageValue : usagePages) {
        QJsonObject pageObject = pageValue.toObject();
        if (pageObject.value("Id").toInt() == usagePage) {
            const QString usagePageStr = pageObject.value("Name").toString();
            const QJsonArray usageIds = pageObject.value("UsageIds").toArray();
            for (const QJsonValue& usageValue : usageIds) {
                QJsonObject usageObject = usageValue.toObject();
                if (usageObject.value("Id").toInt() == usage) {
                    return usageObject.value("Name").toString();
                }
            }
            break; // No need to continue if the usage page is found
        }
    }
    return QStringLiteral("Reserved");
}

DeviceInfo::DeviceInfo(
        const hid_device_info& device_info, const HidUsageTables& hidUsageTables)
        : vendor_id(device_info.vendor_id),
          product_id(device_info.product_id),
          release_number(device_info.release_number),
          usage_page(device_info.usage_page),
          usage(device_info.usage),
          m_usbInterfaceNumber(device_info.interface_number),
          m_pathRaw(device_info.path, mixxx::strnlen_s(device_info.path, PATH_MAX)),
          m_serialNumberRaw(device_info.serial_number,
                  mixxx::wcsnlen_s(device_info.serial_number,
                          kDeviceInfoStringMaxLength)),
          m_manufacturerString(mixxx::convertWCStringToQString(
                  device_info.manufacturer_string,
                  kDeviceInfoStringMaxLength)),
          m_productString(mixxx::convertWCStringToQString(device_info.product_string,
                  kDeviceInfoStringMaxLength)),
          m_serialNumber(mixxx::convertWCStringToQString(
                  m_serialNumberRaw.data(), m_serialNumberRaw.size())),
          m_hidUsageTables(hidUsageTables) {
    switch (device_info.bus_type) {
    case HID_API_BUS_USB:
        m_physicalTransportProtocol = PhysicalTransportProtocol::USB;
        break;
    case HID_API_BUS_BLUETOOTH:
        m_physicalTransportProtocol = PhysicalTransportProtocol::BlueTooth;
        break;
    case HID_API_BUS_I2C:
        m_physicalTransportProtocol = PhysicalTransportProtocol::I2C;
        break;
    case HID_API_BUS_SPI:
        m_physicalTransportProtocol = PhysicalTransportProtocol::SPI;
        break;
    default:
        m_physicalTransportProtocol = PhysicalTransportProtocol::UNKNOWN;
        break;
    }
}

QString DeviceInfo::formatName() const {
    // We include the last 4 digits of the serial number and the
    // interface number to allow the user (and Mixxx!) to keep
    // track of which is which
    const auto serialSuffix = getSerialNumber().right(4);
    if (m_usbInterfaceNumber >= 0) {
        return getProductString() +
                QChar(' ') +
                serialSuffix +
                QChar('_') +
                QString::number(m_usbInterfaceNumber);
    } else {
        return getProductString() +
                QChar(' ') +
                serialSuffix;
    }
}

QDebug operator<<(QDebug dbg, const DeviceInfo& deviceInfo) {
    QStringList parts;
    parts.reserve(8);

    auto formatHex = [](unsigned short value) {
        return QString::number(value, 16).toUpper().rightJustified(4, '0');
    };

    parts.append(QStringLiteral("VID:%1 PID:%2")
                    .arg(formatHex(deviceInfo.getVendorId()))
                    .arg(formatHex(deviceInfo.getProductId())));

    static const QMap<PhysicalTransportProtocol, QString> protocolMap = {
            {PhysicalTransportProtocol::USB, QStringLiteral("USB")},
            {PhysicalTransportProtocol::BlueTooth, QStringLiteral("Bluetooth")},
            {PhysicalTransportProtocol::I2C, QStringLiteral("I2C")},
            {PhysicalTransportProtocol::SPI, QStringLiteral("SPI")},
            {PhysicalTransportProtocol::FireWire, QStringLiteral("Firewire")},
            {PhysicalTransportProtocol::UNKNOWN, QStringLiteral("Unknown")}};

    parts.append(QStringLiteral("Physical: %1")
                    .arg(protocolMap.value(
                            deviceInfo.getPhysicalTransportProtocol(),
                            QStringLiteral("Unknown"))));

    parts.append(QStringLiteral("Usage-Page: %1 %2")
                    .arg(formatHex(deviceInfo.getUsagePage()))
                    .arg(deviceInfo.getUsagePageDescription()));
    parts.append(QStringLiteral("Usage: %1 %2")
                    .arg(formatHex(deviceInfo.getUsage()))
                    .arg(deviceInfo.getUsageDescription()));

    if (deviceInfo.getUsbInterfaceNumber()) {
        parts.append(QStringLiteral("Interface: #%1")
                        .arg(deviceInfo.getUsbInterfaceNumber().value()));
    }
    if (!deviceInfo.getManufacturerString().isEmpty()) {
        parts.append(QStringLiteral("Manufacturer: ") + deviceInfo.getManufacturerString());
    }
    if (!deviceInfo.getProductString().isEmpty()) {
        parts.append(QStringLiteral("Product: ") + deviceInfo.getProductString());
    }
    if (!deviceInfo.getSerialNumber().isEmpty()) {
        parts.append(QStringLiteral("S/N: ") + deviceInfo.getSerialNumber());
    }

    const auto dbgState = QDebugStateSaver(dbg);
    return dbg.nospace().noquote()
            << QStringLiteral("{ ")
            << parts.join(QStringLiteral(" | "))
            << QStringLiteral(" }");
}

bool DeviceInfo::matchProductInfo(
        const ProductInfo& product) const {
    bool ok;
    // Product and vendor match is always required
    if (vendor_id != product.vendor_id.toInt(&ok, 16) || !ok) {
        return false;
    }
    if (product_id != product.product_id.toInt(&ok, 16) || !ok) {
        return false;
    }

    // Optionally check against m_usbInterfaceNumber / usage_page && usage
    if (m_usbInterfaceNumber >= 0) {
        if (m_usbInterfaceNumber != product.interface_number.toInt(&ok, 16) || !ok) {
            return false;
        }
    } else {
        if (usage_page != product.usage_page.toInt(&ok, 16) || !ok) {
            return false;
        }
        if (usage != product.usage.toInt(&ok, 16) || !ok) {
            return false;
        }
    }
    // Match found
    return true;
}

} // namespace hid

} // namespace mixxx
