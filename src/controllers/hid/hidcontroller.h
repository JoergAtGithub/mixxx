#pragma once

#include <QThread>
#include <QMutexLocker>

#include "controllers/controller.h"
#include "controllers/hid/hiddevice.h"
#include "controllers/hid/legacyhidcontrollermapping.h"
#include "util/duration.h"


namespace {
   static constexpr int kBufferSize = 255;
} // namespace


class HidReport : public QObject {
    Q_OBJECT
  public:
    HidReport(const unsigned char& reportId, hid_device* device, const QString device_name, const wchar_t* device_serial_number, const RuntimeLoggingCategory& logOutput);
    virtual ~HidReport();

    void sendOutputReport(QByteArray data, unsigned int reportID);
  signals:
    void sendBytesReport(QByteArray data, unsigned int reportID);

  private:
    const unsigned char m_reportId;
    hid_device* m_pHidDevice;
    const QString m_pHidDeviceName;
    const wchar_t* m_pHidDeviceSerialNumber;
    const RuntimeLoggingCategory m_logOutput;
};

class HidIoReport : public QObject {
    Q_OBJECT
  public:
    HidIoReport(const unsigned char& reportId, hid_device* device, const QString device_name, const wchar_t* device_serial_number, const RuntimeLoggingCategory& logOutput);
    virtual ~HidIoReport();

  public slots:
    void sendBytesReport(QByteArray data, unsigned int reportID);

  private:
    QByteArray m_lastSentOutputreport;
    const unsigned char m_reportId;
    hid_device* m_pHidDevice;
    const QString m_pHidDeviceName;
    const wchar_t* m_pHidDeviceSerialNumber;
    const RuntimeLoggingCategory m_logOutput;
    const QObject* m_pParent;
};


class HidIo : public QThread {
    Q_OBJECT
  public:
    HidIo(hid_device* device, const QString, const wchar_t*, const RuntimeLoggingCategory& logBase, const RuntimeLoggingCategory& logInput, const RuntimeLoggingCategory& logOutput);
    virtual ~HidIo();

    void stop() {
        m_stop = 1;
    }
    mutable QMutex m_HidIoMutex;

    std::unique_ptr<HidIoReport> m_outputReport[256];

    static constexpr int kNumBuffers = 2;
    unsigned char m_pPollData[kNumBuffers][kBufferSize];
    int m_lastPollSize;
    int m_pollingBufferIndex;
    const RuntimeLoggingCategory m_logBase;
    const RuntimeLoggingCategory m_logInput;
    const RuntimeLoggingCategory m_logOutput;

  signals:
    /// Signals that a HID InputReport received by Interupt triggered from HID device
    void receive(const QByteArray& data, mixxx::Duration timestamp);

  public slots:
    QByteArray getInputReport(unsigned int reportID);
    void sendFeatureReport(const QByteArray& reportData, unsigned int reportID);
    QByteArray getFeatureReport(unsigned int reportID);

  protected:
    void run();

  private:
    void processInputReport(int bytesRead);
    hid_device* m_pHidDevice;
    const QString m_pHidDeviceName;
    const wchar_t* m_pHidDeviceSerialNumber;
    QAtomicInt m_stop;
};

/// HID controller backend
class HidController final : public Controller {
    Q_OBJECT
  public:
    explicit HidController(
            mixxx::hid::DeviceInfo&& deviceInfo);
    ~HidController() override;

    ControllerJSProxy* jsProxy() override;

    std::unique_ptr<HidReport> m_outputReport[256];

    QString mappingExtension() override;

    virtual std::shared_ptr<LegacyControllerMapping> cloneMapping() override;
    void setMapping(std::shared_ptr<LegacyControllerMapping> pMapping) override;

    bool isMappable() const override {
        if (!m_pMapping) {
            return false;
        }
        return m_pMapping->isMappable();
    }

    bool matchMapping(const MappingInfo& mapping) override;

  protected:
    void sendReport(QList<int> data, unsigned int length, unsigned int reportID);

  private slots:
    int open() override;
    int close() override;

  signals:
    // getInputReport receives an input report on request.
    // This can be used on startup to initialize the knob positions in Mixxx
    // to the physical position of the hardware knobs on the controller.
    // The returned data structure for the input reports is the same
    // as in the polling functionality (including ReportID in first byte).
    // The returned list can be used to call the incomingData
    // function of the common-hid-packet-parser.
    QByteArray getInputReport(unsigned int reportID);

    void sendFeatureReport(const QByteArray& reportData, unsigned int reportID);

    // getFeatureReport receives a feature reports on request.
    // HID doesn't support polling feature reports, therefore this is the
    // only method to get this information.
    // Usually, single bits in a feature report need to be set without
    // changing the other bits. The returned list matches the input
    // format of sendFeatureReport, allowing it to be read, modified
    // and sent it back to the controller.
    QByteArray getFeatureReport(unsigned int reportID);

  private:

    // For devices which only support a single report, reportID must be set to
    // 0x0.
    void sendBytes(const QByteArray& data) override;
  
    const mixxx::hid::DeviceInfo m_deviceInfo;

    HidIo* m_pHidIo;
    hid_device* m_pHidDevice;
    std::shared_ptr<LegacyHidControllerMapping> m_pMapping;

    friend class HidControllerJSProxy;
};

class HidControllerJSProxy : public ControllerJSProxy {
    Q_OBJECT
  public:
    HidControllerJSProxy(HidController* m_pController)
            : ControllerJSProxy(m_pController),
              m_pHidController(m_pController) {
    }

    Q_INVOKABLE void send(const QList<int>& data, unsigned int length = 0) override {
        m_pHidController->send(data, length);
    }

    Q_INVOKABLE void send(const QList<int>& data, unsigned int length, unsigned int reportID) {
        m_pHidController->sendReport(data, length, reportID);
    }

    Q_INVOKABLE QByteArray getInputReport(
            unsigned int reportID) {
        return m_pHidController->getInputReport(reportID);
    }

    Q_INVOKABLE void sendFeatureReport(
            const QByteArray& reportData, unsigned int reportID) {
        m_pHidController->sendFeatureReport(reportData, reportID);
    }

    Q_INVOKABLE QByteArray getFeatureReport(
            unsigned int reportID) {
        return m_pHidController->getFeatureReport(reportID);
    }

  private:
    HidController* m_pHidController;
};
