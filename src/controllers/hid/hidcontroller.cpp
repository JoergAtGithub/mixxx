#include "controllers/hid/hidcontroller.h"

#include <hidapi.h>

#include "controllers/defs_controllers.h"
#include "controllers/hid/legacyhidcontrollermappingfilehandler.h"
#include "moc_hidcontroller.cpp"
#include "util/string.h"
#include "util/time.h"
#include "util/trace.h"

namespace {
constexpr int kReportIdSize = 1;
constexpr int kMaxHidErrorMessageSize = 512;
} // namespace

HidIoReport::HidIoReport(const unsigned char& reportId, hid_device* device, const QString device_name, const wchar_t* device_serial_number, const RuntimeLoggingCategory& logOutput)
        : m_reportId(reportId),
          m_pHidDevice(device),
          m_pHidDeviceName(device_name),
          m_pHidDeviceSerialNumber(device_serial_number),
          m_logOutput(logOutput) {
}

HidIoReport::~HidIoReport() {
}

HidReport::HidReport(const unsigned char& reportId, hid_device* device, const QString device_name, const wchar_t* device_serial_number, const RuntimeLoggingCategory& logOutput)
        : m_reportId(reportId),
          m_pHidDevice(device),
          m_pHidDeviceName(device_name),
          m_pHidDeviceSerialNumber(device_serial_number),
          m_logOutput(logOutput) {
}

HidReport::~HidReport() {
}


HidIO::HidIO(hid_device* device, const QString device_name, const wchar_t* device_serial_number, const RuntimeLoggingCategory& logBase, const RuntimeLoggingCategory& logInput, const RuntimeLoggingCategory& logOutput)
        : QThread(),
          m_pHidDevice(device),
          m_pHidDeviceName(device_name),
          m_pHidDeviceSerialNumber(device_serial_number),
          m_pollingBufferIndex(0),
          m_logBase(logBase),
          m_logInput(logInput),
          m_logOutput(logOutput)
        {

    // This isn't strictly necessary but is good practice.
    for (int i = 0; i < kNumBuffers; i++) {
        memset(m_pPollData[i], 0, kBufferSize);
    }
    m_lastPollSize = 0;
    
    for (int i = 0; i <= 255; i++) {
        m_outputReport[i] = std::make_unique<HidIoReport>(i, device, device_name, device_serial_number, logOutput);
    }
}

HidIO::~HidIO() {
}

void HidIO::run() {
    m_stop = 0;

    while (m_stop.loadRelaxed() == 0) {
        // hid_read_timeout reads an Input Report from a HID device.
        // If no packet was available to be read within
        // the timeout period, this function returns 0.
        Trace hidio_run("HidIO read interupt based IO");
        for (int i = 0; i < 512; i++) {
            int bytesRead = hid_read(m_pHidDevice, m_pPollData[m_pollingBufferIndex], kBufferSize);
            //int bytesRead = hid_read_timeout(m_pHidDevice, m_pPollData[m_pollingBufferIndex], kBufferSize, 1);
            Trace hidio_run2("HidIO process packet");
            if (bytesRead < 0) {
                // -1 is the only error value according to hidapi documentation.
                qCWarning(m_logOutput) << "Unable to read data from" << m_pHidDeviceName << ":"
                                       << mixxx::convertWCStringToQString(
                                                  hid_error(m_pHidDevice),
                                                  kMaxHidErrorMessageSize);
                DEBUG_ASSERT(bytesRead == -1);
            } else if (bytesRead == 0) {
                // No packet was available to be read -> HID ring buffer completly read
                // Ring buffer size differs by hidapi backend: libusb(30 reports), mac(30 report), windows(64 reports), hidraw(2048 bytes)
                //if (i >= 30)
                //    qWarning() << "HID controller: " << m_pHidDeviceName << " Serial #" << m_pHidDeviceSerialNumber << " HID Ring buffer critical filled -> InputReports might be popped off -> Events by state transitions might be missed in the mapping. (Ring buffers received: " << i << ")";
                break;
            } else {
                processInputReport(bytesRead);
            }
        }
        //QCoreApplication::processEvents();
        usleep(500);
    }
}

void HidIO::processInputReport(int bytesRead) {
    Trace process("HidIO processInputReport");
    unsigned char* pPreviousBuffer = m_pPollData[(m_pollingBufferIndex + 1) % kNumBuffers];
    unsigned char* pCurrentBuffer = m_pPollData[m_pollingBufferIndex];
    // Some controllers such as the Gemini GMX continuously send input reports even if it
    // is identical to the previous send input report. If this loop processed all those redundant
    // input report, it would be a big performance problem to run JS code for every  input report and
    // would be unnecessary.
    // This assumes that the redundant input report all use the same report ID. In practice we
    // have not encountered any controllers that send redundant input report with different report
    // IDs. If any such devices exist, this may be changed to use a separate buffer to store
    // the last input report for each report ID.
    if (bytesRead == m_lastPollSize &&
            memcmp(pCurrentBuffer, pPreviousBuffer, bytesRead) == 0) {
        return;
    }
    // Cycle between buffers so the memcmp above does not require deep copying to another buffer.
    m_pollingBufferIndex = (m_pollingBufferIndex + 1) % kNumBuffers;
    m_lastPollSize = bytesRead;
    auto incomingData = QByteArray::fromRawData(
            reinterpret_cast<char*>(pCurrentBuffer), bytesRead);

    // Execute callback function in JavaScript mapping
    // and print to stdout in case of --controllerDebug
    emit(receive(incomingData, mixxx::Time::elapsed()));
}

QByteArray HidIO::getInputReport(unsigned int reportID) {
    Trace hidRead("HidIO getInputReport");

    int bytesRead = 0;

    m_pPollData[m_pollingBufferIndex][0] = reportID;

    // FIXME: implement upstream for hidraw backend on Linux
    // https://github.com/libusb/hidapi/issues/259
    bytesRead = hid_get_input_report(m_pHidDevice, m_pPollData[m_pollingBufferIndex], kBufferSize);
    qCDebug(m_logInput) << bytesRead
                        << "bytes received by hid_get_input_report" << m_pHidDeviceName
                        << "serial #" << m_pHidDeviceSerialNumber
                        << "(including one byte for the report ID:"
                        << QString::number(static_cast<quint8>(reportID), 16)
                                   .toUpper()
                                   .rightJustified(2, QChar('0'))
                        << ")";
    if (bytesRead <= kReportIdSize) {
        // -1 is the only error value according to hidapi documentation.
        // Otherwise minimum possible value is 1, because 1 byte is for the reportID,
        // the smallest report with data is therefore 2 bytes.
        DEBUG_ASSERT(bytesRead <= kReportIdSize);
        return QByteArray();
    }

    return QByteArray::fromRawData(
            reinterpret_cast<char*>(m_pPollData[m_pollingBufferIndex]), bytesRead);
}

void HidIoReport::sendBytesReport(QByteArray data, unsigned int reportID) {
    // Append the Report ID to the beginning of data[] per the API..
    data.prepend(reportID);
    auto startOfHidWrite = mixxx::Time::elapsed();
    int result = hid_write(m_pHidDevice, (unsigned char*)data.constData(), data.size());
    if (result == -1) {
        qCWarning(m_logOutput) << "Unable to send data to" << m_pHidDeviceName << ":"
                               << mixxx::convertWCStringToQString(
                                          hid_error(m_pHidDevice),
                                          kMaxHidErrorMessageSize);
    } else {
        qCDebug(m_logOutput) << "t:" << startOfHidWrite.formatMillisWithUnit() << " "
                             << result << "bytes sent to" << m_pHidDeviceName
                             << "serial #" << m_pHidDeviceSerialNumber
                             << "(including report ID of" << reportID << ") - Needed: "
                             << (mixxx::Time::elapsed() - startOfHidWrite).formatMicrosWithUnit();
    }
}


void HidIO::sendFeatureReport(
        const QByteArray& reportData, unsigned int reportID) {
    QByteArray dataArray;
    dataArray.reserve(kReportIdSize + reportData.size());

    // Append the Report ID to the beginning of dataArray[] per the API..
    dataArray.append(reportID);

    for (const int datum : reportData) {
        dataArray.append(datum);
    }

    int result = hid_send_feature_report(m_pHidDevice,
            reinterpret_cast<const unsigned char*>(dataArray.constData()),
            dataArray.size());
    if (result == -1) {
        qCWarning(m_logOutput) << "sendFeatureReport is unable to send data to"
                               << m_pHidDeviceName << "serial #" << m_pHidDeviceSerialNumber
                               << ":"
                               << mixxx::convertWCStringToQString(
                                          hid_error(m_pHidDevice),
                                          kMaxHidErrorMessageSize);
    } else {
        qCDebug(m_logOutput) << result << "bytes sent by sendFeatureReport to" << m_pHidDeviceName
                             << "serial #" << m_pHidDeviceSerialNumber
                             << "(including report ID of" << reportID << ")";
    }
}


QByteArray HidIO::getFeatureReport(
        unsigned int reportID) {
    unsigned char dataRead[kReportIdSize + kBufferSize];
    dataRead[0] = reportID;

    int bytesRead;
    bytesRead = hid_get_feature_report(m_pHidDevice,
            dataRead,
            kReportIdSize + kBufferSize);
    if (bytesRead <= kReportIdSize) {
        // -1 is the only error value according to hidapi documentation.
        // Otherwise minimum possible value is 1, because 1 byte is for the reportID,
        // the smallest report with data is therefore 2 bytes.
        qCWarning(m_logInput) << "getFeatureReport is unable to get data from" << m_pHidDeviceName
                              << "serial #" << m_pHidDeviceSerialNumber << ":"
                              << mixxx::convertWCStringToQString(
                                         hid_error(m_pHidDevice),
                                         kMaxHidErrorMessageSize);
    } else {
        qCDebug(m_logInput) << bytesRead
                            << "bytes received by getFeatureReport from" << m_pHidDeviceName
                            << "serial #" << m_pHidDeviceSerialNumber
                            << "(including one byte for the report ID:"
                            << QString::number(static_cast<quint8>(reportID), 16)
                                       .toUpper()
                                       .rightJustified(2, QChar('0'))
                            << ")";
    }

    // Convert array of bytes read in a JavaScript compatible return type
    // For compatibility with input array HidController::sendFeatureReport, a reportID prefix is not added here
    QByteArray byteArray;
    byteArray.reserve(bytesRead - kReportIdSize);
    const auto featureReportStart = reinterpret_cast<const char*>(dataRead + kReportIdSize);
    return QByteArray(featureReportStart, bytesRead);
}

HidController::HidController(
        mixxx::hid::DeviceInfo&& deviceInfo)
        : Controller(deviceInfo.formatName()),
          m_deviceInfo(std::move(deviceInfo)),
          m_pHidDevice(nullptr) {
    setDeviceCategory(mixxx::hid::DeviceCategory::guessFromDeviceInfo(m_deviceInfo));

    // All HID devices are full-duplex
    setInputDevice(true);
    setOutputDevice(true);

    m_pHidInteruptIn = nullptr;
    m_pHidInteruptOut = nullptr;
        
}

HidController::~HidController() {
    if (isOpen()) {
        close();
    }
}

QString HidController::mappingExtension() {
    return HID_MAPPING_EXTENSION;
}

void HidController::setMapping(std::shared_ptr<LegacyControllerMapping> pMapping) {
    m_pMapping = downcastAndTakeOwnership<LegacyHidControllerMapping>(std::move(pMapping));
}

std::shared_ptr<LegacyControllerMapping> HidController::cloneMapping() {
    if (!m_pMapping) {
        return nullptr;
    }
    return m_pMapping->clone();
}

bool HidController::matchMapping(const MappingInfo& mapping) {
    const QList<ProductInfo>& products = mapping.getProducts();
    for (const auto& product : products) {
        if (m_deviceInfo.matchProductInfo(product)) {
            return true;
        }
    }
    return false;
}

int HidController::open() {
    if (isOpen()) {
        qDebug() << "HID device" << getName() << "already open";
        return -1;
    }

    // Open device by path
    qCInfo(m_logBase) << "Opening HID device" << getName() << "by HID path"
                      << m_deviceInfo.pathRaw();

    m_pHidDevice = hid_open_path(m_deviceInfo.pathRaw());

    // If that fails, try to open device with vendor/product/serial #
    if (!m_pHidDevice) {
        qCWarning(m_logBase) << "Failed. Trying to open with make, model & serial no:"
                             << m_deviceInfo.vendorId() << m_deviceInfo.productId()
                             << m_deviceInfo.serialNumber();
        m_pHidDevice = hid_open(
                m_deviceInfo.vendorId(),
                m_deviceInfo.productId(),
                m_deviceInfo.serialNumberRaw());
    }

    // If it does fail, try without serial number WARNING: This will only open
    // one of multiple identical devices
    if (!m_pHidDevice) {
        qCWarning(m_logBase) << "Unable to open specific HID device" << getName()
                             << "Trying now with just make and model."
                             << "(This may only open the first of multiple identical devices.)";
        m_pHidDevice = hid_open(m_deviceInfo.vendorId(),
                m_deviceInfo.productId(),
                nullptr);
    }

    // If that fails, we give up!
    if (!m_pHidDevice) {
        qCWarning(m_logBase) << "Unable to open HID device" << getName();
        return -1;
    }

    // Set hid controller to non-blocking
    if (hid_set_nonblocking(m_pHidDevice, 1) != 0) {
        qCWarning(m_logBase) << "Unable to set HID device " << getName() << " to non-blocking";
        return -1;
    }

    setOpen(true);
    startEngine();

    if (m_pHidInteruptIn != nullptr) {
        qWarning() << "HidIO already present for" << getName();
    } else {
        m_pHidInteruptIn = new HidIO(m_pHidDevice, getName(), m_deviceInfo.serialNumberRaw(), m_logBase, m_logInput, m_logOutput);
        m_pHidInteruptIn->setObjectName(QString("HidIO %1").arg(getName()));

        connect(m_pHidInteruptIn,
                &HidIO::receive,
                this,
                &HidController::receive,
                Qt::QueuedConnection);

        connect(this,
                &HidController::getInputReport,
                m_pHidInteruptIn,
                &HidIO::getInputReport,
                Qt::DirectConnection);

        connect(this,
                &HidController::getFeatureReport,
                m_pHidInteruptIn,
                &HidIO::getFeatureReport,
                Qt::DirectConnection);
        connect(this,
                &HidController::sendFeatureReport,
                m_pHidInteruptIn,
                &HidIO::sendFeatureReport,
                Qt::DirectConnection);

        // Controller input needs to be prioritized since it can affect the
        // audio directly, like when scratching
        m_pHidInteruptIn->start(QThread::HighPriority);
    }

    
    for (int i = 0; i <= 255; i++) {
        m_outputReport[i] = std::make_unique<HidReport>(i, m_pHidDevice, getName(), m_deviceInfo.serialNumberRaw(), m_logOutput);
    }

    if (m_pHidInteruptOut != nullptr) {
        qWarning() << "HidIO already present for" << getName();
    } else {
        m_pHidInteruptOut = new HidIO(m_pHidDevice, getName(), m_deviceInfo.serialNumberRaw(), m_logBase, m_logInput, m_logOutput);
        m_pHidInteruptOut->setObjectName(QString("HidIO %1").arg(getName()));

        for (int i = 0; i <= 255; i++) {
            connect(m_outputReport[i].get(),
                    &HidReport::sendBytesReport,
                    m_pHidInteruptOut->m_outputReport[i].get(),
                    &HidIoReport::sendBytesReport,
                    Qt::QueuedConnection);                
        }

        // Controller input needs to be prioritized since it can affect the
        // audio directly, like when scratching
        m_pHidInteruptOut->start(QThread::NormalPriority);
    }
    return 0;
}

int HidController::close() {
    if (!isOpen()) {
        qCWarning(m_logBase) << "HID device" << getName() << "already closed";
        return -1;
    }

    qCInfo(m_logBase) << "Shutting down HID device" << getName();

    
    // Stop the reading thread
    if (m_pHidInteruptIn == nullptr) {
        qWarning() << "HidIO not present for" << getName()
                   << "yet the device is open!";
    } else {
        disconnect(m_pHidInteruptIn,
                &HidIO::receive,
                this,
                &HidController::receive);

        disconnect(this,
                &HidController::getInputReport,
                m_pHidInteruptIn,
                &HidIO::getInputReport);

        disconnect(this,
                &HidController::getFeatureReport,
                m_pHidInteruptIn,
                &HidIO::getFeatureReport);
        disconnect(this,
                &HidController::sendFeatureReport,
                m_pHidInteruptIn,
                &HidIO::sendFeatureReport);

        m_pHidInteruptIn->stop();
        hid_set_nonblocking(m_pHidDevice, 1); // Quit blocking
        qDebug() << "Waiting on IO thread to finish";
        m_pHidInteruptIn->wait();
    }

    // Stop the writing thread
    if (m_pHidInteruptOut == nullptr) {
        qWarning() << "HidIO not present for" << getName()
                   << "yet the device is open!";
    } else {
        /* disconnect(this,
                &HidController::sendBytesReport,
                m_pHidInteruptOut,
                &HidIO::sendBytesReport);*/

        m_pHidInteruptOut->stop();
        hid_set_nonblocking(m_pHidDevice, 1); // Quit blocking
        qDebug() << "Waiting on IO thread to finish";
        m_pHidInteruptOut->wait();
    }

    // Stop controller engine here to ensure it's done before the device is closed
    // in case it has any final parting messages
    stopEngine();

    if (m_pHidInteruptIn != nullptr) {
        delete m_pHidInteruptIn;
        m_pHidInteruptIn = nullptr;
    }
    if (m_pHidInteruptOut != nullptr) {
        delete m_pHidInteruptOut;
        m_pHidInteruptOut = nullptr;
    }

    // Close device
    qCInfo(m_logBase) << "Closing device";
    // hid_close is not thread safe
    // All communication to this hid_device must be completed, before hid_close is called.
    hid_close(m_pHidDevice);
    setOpen(false);
    return 0;
}


void HidController::sendReport(QList<int> data, unsigned int length, unsigned int reportID) {
    Q_UNUSED(length);
    QByteArray temp;
    foreach (int datum, data) {
        temp.append(datum);
    }
    m_outputReport[reportID]->sendOutputReport(temp, reportID);
}

void HidController::sendBytes(const QByteArray& data) {
    m_outputReport[0]->sendOutputReport(data, 0);
}

void HidReport::sendOutputReport(QByteArray data, unsigned int reportID) {
    emit(sendBytesReport(data, reportID));
}


ControllerJSProxy* HidController::jsProxy() {
    return new HidControllerJSProxy(this);
}

